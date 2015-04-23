// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_sync/background_sync_manager.h"

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "content/browser/background_sync/background_sync_network_observer.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "content/public/browser/browser_thread.h"

namespace {
const char kBackgroundSyncUserDataKey[] = "BackgroundSyncUserData";
}

namespace content {

const BackgroundSyncManager::BackgroundSyncRegistration::RegistrationId
    BackgroundSyncManager::BackgroundSyncRegistration::kInvalidRegistrationId =
        -1;

const BackgroundSyncManager::BackgroundSyncRegistration::RegistrationId
    BackgroundSyncManager::BackgroundSyncRegistration::kInitialId = 0;

BackgroundSyncManager::BackgroundSyncRegistrations::
    BackgroundSyncRegistrations()
    : next_id(BackgroundSyncRegistration::kInitialId) {
}

BackgroundSyncManager::BackgroundSyncRegistrations::
    ~BackgroundSyncRegistrations() {
}

// static
scoped_ptr<BackgroundSyncManager> BackgroundSyncManager::Create(
    const scoped_refptr<ServiceWorkerContextWrapper>& service_worker_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  BackgroundSyncManager* sync_manager =
      new BackgroundSyncManager(service_worker_context);
  sync_manager->Init();
  return make_scoped_ptr(sync_manager);
}

BackgroundSyncManager::~BackgroundSyncManager() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  service_worker_context_->RemoveObserver(this);
}

BackgroundSyncManager::RegistrationKey::RegistrationKey(
    const BackgroundSyncRegistration& registration)
    : RegistrationKey(registration.tag, registration.periodicity) {
}

BackgroundSyncManager::RegistrationKey::RegistrationKey(
    const std::string& tag,
    SyncPeriodicity periodicity)
    : value_(periodicity == SYNC_ONE_SHOT ? "o_" + tag : "p_" + tag) {
}

void BackgroundSyncManager::Register(
    const GURL& origin,
    int64 sw_registration_id,
    const BackgroundSyncRegistration& sync_registration,
    const StatusAndRegistrationCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_EQ(BackgroundSyncRegistration::kInvalidRegistrationId,
            sync_registration.id);

  if (disabled_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, ERROR_TYPE_STORAGE, BackgroundSyncRegistration()));
    return;
  }

  op_scheduler_.ScheduleOperation(base::Bind(
      &BackgroundSyncManager::RegisterImpl, weak_ptr_factory_.GetWeakPtr(),
      origin, sw_registration_id, sync_registration,
      MakeStatusAndRegistrationCompletion(callback)));
}

void BackgroundSyncManager::Unregister(
    const GURL& origin,
    int64 sw_registration_id,
    const std::string& sync_registration_tag,
    SyncPeriodicity periodicity,
    BackgroundSyncRegistration::RegistrationId sync_registration_id,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (disabled_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERROR_TYPE_STORAGE));
    return;
  }

  RegistrationKey registration_key(sync_registration_tag, periodicity);

  op_scheduler_.ScheduleOperation(base::Bind(
      &BackgroundSyncManager::UnregisterImpl, weak_ptr_factory_.GetWeakPtr(),
      origin, sw_registration_id, registration_key, sync_registration_id,
      MakeStatusCompletion(callback)));
}

void BackgroundSyncManager::GetRegistration(
    const GURL& origin,
    int64 sw_registration_id,
    const std::string sync_registration_tag,
    SyncPeriodicity periodicity,
    const StatusAndRegistrationCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (disabled_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, ERROR_TYPE_STORAGE, BackgroundSyncRegistration()));
    return;
  }

  RegistrationKey registration_key(sync_registration_tag, periodicity);

  op_scheduler_.ScheduleOperation(base::Bind(
      &BackgroundSyncManager::GetRegistrationImpl,
      weak_ptr_factory_.GetWeakPtr(), origin, sw_registration_id,
      registration_key, MakeStatusAndRegistrationCompletion(callback)));
}

void BackgroundSyncManager::OnRegistrationDeleted(int64 registration_id,
                                                  const GURL& pattern) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Operations already in the queue will either fail when they write to storage
  // or return stale results based on registrations loaded in memory. This is
  // inconsequential since the service worker is gone.
  op_scheduler_.ScheduleOperation(base::Bind(
      &BackgroundSyncManager::OnRegistrationDeletedImpl,
      weak_ptr_factory_.GetWeakPtr(), registration_id, MakeEmptyCompletion()));
}

void BackgroundSyncManager::OnStorageWiped() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Operations already in the queue will either fail when they write to storage
  // or return stale results based on registrations loaded in memory. This is
  // inconsequential since the service workers are gone.
  op_scheduler_.ScheduleOperation(
      base::Bind(&BackgroundSyncManager::OnStorageWipedImpl,
                 weak_ptr_factory_.GetWeakPtr(), MakeEmptyCompletion()));
}

BackgroundSyncManager::BackgroundSyncManager(
    const scoped_refptr<ServiceWorkerContextWrapper>& service_worker_context)
    : service_worker_context_(service_worker_context),
      disabled_(false),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  service_worker_context_->AddObserver(this);

  network_observer_.reset(new BackgroundSyncNetworkObserver(
      base::Bind(&BackgroundSyncManager::OnNetworkChanged,
                 weak_ptr_factory_.GetWeakPtr())));
}

void BackgroundSyncManager::Init() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!op_scheduler_.ScheduledOperations());
  DCHECK(!disabled_);

  op_scheduler_.ScheduleOperation(base::Bind(&BackgroundSyncManager::InitImpl,
                                             weak_ptr_factory_.GetWeakPtr(),
                                             MakeEmptyCompletion()));
}

void BackgroundSyncManager::InitImpl(const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (disabled_) {
    base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback));
    return;
  }

  GetDataFromBackend(
      kBackgroundSyncUserDataKey,
      base::Bind(&BackgroundSyncManager::InitDidGetDataFromBackend,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void BackgroundSyncManager::InitDidGetDataFromBackend(
    const base::Closure& callback,
    const std::vector<std::pair<int64, std::string>>& user_data,
    ServiceWorkerStatusCode status) {
  if (status != SERVICE_WORKER_OK && status != SERVICE_WORKER_ERROR_NOT_FOUND) {
    LOG(ERROR) << "BackgroundSync failed to init due to backend failure.";
    DisableAndClearManager(base::Bind(callback));
    return;
  }

  bool corruption_detected = false;
  for (const std::pair<int64, std::string>& data : user_data) {
    BackgroundSyncRegistrationsProto registrations_proto;
    if (registrations_proto.ParseFromString(data.second)) {
      BackgroundSyncRegistrations* registrations =
          &sw_to_registrations_map_[data.first];
      registrations->next_id = registrations_proto.next_registration_id();
      registrations->origin = GURL(registrations_proto.origin());

      for (int i = 0, max = registrations_proto.registration_size(); i < max;
           ++i) {
        const BackgroundSyncRegistrationProto& registration_proto =
            registrations_proto.registration(i);

        if (registration_proto.id() >= registrations->next_id) {
          corruption_detected = true;
          break;
        }

        RegistrationKey registration_key(registration_proto.tag(),
                                         registration_proto.periodicity());
        BackgroundSyncRegistration* registration =
            &registrations->registration_map[registration_key];

        registration->id = registration_proto.id();
        registration->tag = registration_proto.tag();
        registration->periodicity = registration_proto.periodicity();
        registration->min_period = registration_proto.min_period();
        registration->network_state = registration_proto.network_state();
        registration->power_state = registration_proto.power_state();
      }
    }

    if (corruption_detected)
      break;
  }

  if (corruption_detected) {
    LOG(ERROR) << "Corruption detected in background sync backend";
    DisableAndClearManager(base::Bind(callback));
    return;
  }

  // TODO(jkarlin): Call the scheduling algorithm here.

  base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback));
}

void BackgroundSyncManager::RegisterImpl(
    const GURL& origin,
    int64 sw_registration_id,
    const BackgroundSyncRegistration& sync_registration,
    const StatusAndRegistrationCallback& callback) {
  if (disabled_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, ERROR_TYPE_STORAGE, BackgroundSyncRegistration()));
    return;
  }

  const BackgroundSyncRegistration* existing_registration = LookupRegistration(
      sw_registration_id, RegistrationKey(sync_registration));
  if (existing_registration &&
      existing_registration->Equals(sync_registration)) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERROR_TYPE_OK, *existing_registration));
    return;
  }

  BackgroundSyncRegistration new_registration = sync_registration;
  BackgroundSyncRegistrations* registrations =
      &sw_to_registrations_map_[sw_registration_id];
  new_registration.id = registrations->next_id++;

  AddRegistrationToMap(sw_registration_id, origin, new_registration);

  StoreRegistrations(
      sw_registration_id,
      base::Bind(&BackgroundSyncManager::RegisterDidStore,
                 weak_ptr_factory_.GetWeakPtr(), sw_registration_id,
                 new_registration, callback));
}

void BackgroundSyncManager::DisableAndClearManager(
    const base::Closure& callback) {
  if (disabled_) {
    base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback));
    return;
  }

  disabled_ = true;
  sw_to_registrations_map_.clear();

  // Delete all backend entries. The memory representation of registered syncs
  // may be out of sync with storage (e.g., due to corruption detection on
  // loading from storage), so reload the registrations from storage again.
  GetDataFromBackend(
      kBackgroundSyncUserDataKey,
      base::Bind(&BackgroundSyncManager::DisableAndClearDidGetRegistrations,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void BackgroundSyncManager::DisableAndClearDidGetRegistrations(
    const base::Closure& callback,
    const std::vector<std::pair<int64, std::string>>& user_data,
    ServiceWorkerStatusCode status) {
  if (status != SERVICE_WORKER_OK || user_data.empty()) {
    base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback));
    return;
  }

  base::Closure barrier_closure =
      base::BarrierClosure(user_data.size(), base::Bind(callback));

  for (const auto& sw_id_and_regs : user_data) {
    service_worker_context_->ClearRegistrationUserData(
        sw_id_and_regs.first, kBackgroundSyncUserDataKey,
        base::Bind(&BackgroundSyncManager::DisableAndClearManagerClearedOne,
                   weak_ptr_factory_.GetWeakPtr(), barrier_closure));
  }
}

void BackgroundSyncManager::DisableAndClearManagerClearedOne(
    const base::Closure& barrier_closure,
    ServiceWorkerStatusCode status) {
  // The status doesn't matter at this point, there is nothing else to be done.
  base::MessageLoop::current()->PostTask(FROM_HERE,
                                         base::Bind(barrier_closure));
}

BackgroundSyncManager::BackgroundSyncRegistration*
BackgroundSyncManager::LookupRegistration(
    int64 sw_registration_id,
    const RegistrationKey& registration_key) {
  SWIdToRegistrationsMap::iterator it =
      sw_to_registrations_map_.find(sw_registration_id);
  if (it == sw_to_registrations_map_.end())
    return nullptr;

  BackgroundSyncRegistrations& registrations = it->second;
  DCHECK_LE(BackgroundSyncRegistration::kInitialId, registrations.next_id);
  DCHECK(!registrations.origin.is_empty());

  auto key_and_registration_iter =
      registrations.registration_map.find(registration_key);
  if (key_and_registration_iter == registrations.registration_map.end())
    return nullptr;

  return &key_and_registration_iter->second;
}

void BackgroundSyncManager::StoreRegistrations(
    int64 sw_registration_id,
    const ServiceWorkerStorage::StatusCallback& callback) {
  // Serialize the data.
  const BackgroundSyncRegistrations& registrations =
      sw_to_registrations_map_[sw_registration_id];
  BackgroundSyncRegistrationsProto registrations_proto;
  registrations_proto.set_next_registration_id(registrations.next_id);
  registrations_proto.set_origin(registrations.origin.spec());

  for (const auto& key_and_registration : registrations.registration_map) {
    const BackgroundSyncRegistration& registration =
        key_and_registration.second;
    BackgroundSyncRegistrationProto* registration_proto =
        registrations_proto.add_registration();
    registration_proto->set_id(registration.id);
    registration_proto->set_tag(registration.tag);
    registration_proto->set_periodicity(registration.periodicity);
    registration_proto->set_min_period(registration.min_period);
    registration_proto->set_network_state(registration.network_state);
    registration_proto->set_power_state(registration.power_state);
  }
  std::string serialized;
  bool success = registrations_proto.SerializeToString(&serialized);
  DCHECK(success);

  StoreDataInBackend(sw_registration_id, registrations.origin,
                     kBackgroundSyncUserDataKey, serialized, callback);
}

void BackgroundSyncManager::RegisterDidStore(
    int64 sw_registration_id,
    const BackgroundSyncRegistration& new_registration,
    const StatusAndRegistrationCallback& callback,
    ServiceWorkerStatusCode status) {
  if (status == SERVICE_WORKER_ERROR_NOT_FOUND) {
    // The registration is gone.
    sw_to_registrations_map_.erase(sw_registration_id);
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, ERROR_TYPE_STORAGE, BackgroundSyncRegistration()));
    return;
  }

  if (status != SERVICE_WORKER_OK) {
    LOG(ERROR) << "BackgroundSync failed to store registration due to backend "
                  "failure.";
    DisableAndClearManager(
        base::Bind(callback, ERROR_TYPE_STORAGE, BackgroundSyncRegistration()));
    return;
  }

  // TODO(jkarlin): Run the registration algorithm.
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, ERROR_TYPE_OK, new_registration));
}

void BackgroundSyncManager::RemoveRegistrationFromMap(
    int64 sw_registration_id,
    const RegistrationKey& registration_key) {
  DCHECK(LookupRegistration(sw_registration_id, registration_key));

  BackgroundSyncRegistrations* registrations =
      &sw_to_registrations_map_[sw_registration_id];

  registrations->registration_map.erase(registration_key);
}

void BackgroundSyncManager::AddRegistrationToMap(
    int64 sw_registration_id,
    const GURL& origin,
    const BackgroundSyncRegistration& sync_registration) {
  DCHECK_NE(BackgroundSyncRegistration::kInvalidRegistrationId,
            sw_registration_id);

  BackgroundSyncRegistrations* registrations =
      &sw_to_registrations_map_[sw_registration_id];
  registrations->origin = origin;

  RegistrationKey registration_key(sync_registration);
  registrations->registration_map[registration_key] = sync_registration;
}

void BackgroundSyncManager::StoreDataInBackend(
    int64 sw_registration_id,
    const GURL& origin,
    const std::string& backend_key,
    const std::string& data,
    const ServiceWorkerStorage::StatusCallback& callback) {
  service_worker_context_->StoreRegistrationUserData(
      sw_registration_id, origin, backend_key, data, callback);
}

void BackgroundSyncManager::GetDataFromBackend(
    const std::string& backend_key,
    const ServiceWorkerStorage::GetUserDataForAllRegistrationsCallback&
        callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  service_worker_context_->GetUserDataForAllRegistrations(backend_key,
                                                          callback);
}

void BackgroundSyncManager::UnregisterImpl(
    const GURL& origin,
    int64 sw_registration_id,
    const RegistrationKey& registration_key,
    BackgroundSyncRegistration::RegistrationId sync_registration_id,
    const StatusCallback& callback) {
  if (disabled_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERROR_TYPE_STORAGE));
    return;
  }

  const BackgroundSyncRegistration* existing_registration =
      LookupRegistration(sw_registration_id, registration_key);
  if (!existing_registration ||
      existing_registration->id != sync_registration_id) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERROR_TYPE_NOT_FOUND));
    return;
  }

  RemoveRegistrationFromMap(sw_registration_id, registration_key);

  StoreRegistrations(
      sw_registration_id,
      base::Bind(&BackgroundSyncManager::UnregisterDidStore,
                 weak_ptr_factory_.GetWeakPtr(), sw_registration_id, callback));
}

void BackgroundSyncManager::UnregisterDidStore(
    int64 sw_registration_id,
    const StatusCallback& callback,
    ServiceWorkerStatusCode status) {
  if (status == SERVICE_WORKER_ERROR_NOT_FOUND) {
    // ServiceWorker was unregistered.
    sw_to_registrations_map_.erase(sw_registration_id);
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERROR_TYPE_STORAGE));
    return;
  }

  if (status != SERVICE_WORKER_OK) {
    LOG(ERROR) << "BackgroundSync failed to unregister due to backend failure.";
    DisableAndClearManager(base::Bind(callback, ERROR_TYPE_STORAGE));
    return;
  }

  // TODO(jkarlin): Run the registration algorithm.
  base::MessageLoop::current()->PostTask(FROM_HERE,
                                         base::Bind(callback, ERROR_TYPE_OK));
}

void BackgroundSyncManager::GetRegistrationImpl(
    const GURL& origin,
    int64 sw_registration_id,
    const RegistrationKey& registration_key,
    const StatusAndRegistrationCallback& callback) {
  if (disabled_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, ERROR_TYPE_STORAGE, BackgroundSyncRegistration()));
    return;
  }

  const BackgroundSyncRegistration* out_registration =
      LookupRegistration(sw_registration_id, registration_key);
  if (!out_registration) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERROR_TYPE_NOT_FOUND,
                              BackgroundSyncRegistration()));
    return;
  }

  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, ERROR_TYPE_OK, *out_registration));
}

void BackgroundSyncManager::OnRegistrationDeletedImpl(
    int64 registration_id,
    const base::Closure& callback) {
  // The backend (ServiceWorkerStorage) will delete the data, so just delete the
  // memory representation here.
  sw_to_registrations_map_.erase(registration_id);
  base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback));
}

void BackgroundSyncManager::OnStorageWipedImpl(const base::Closure& callback) {
  sw_to_registrations_map_.clear();
  disabled_ = false;
  InitImpl(callback);
}

void BackgroundSyncManager::OnNetworkChanged() {
  // TODO(jkarlin): Run the scheduling algorithm here if initialized and not
  // disabled.
}

void BackgroundSyncManager::PendingStatusAndRegistrationCallback(
    const StatusAndRegistrationCallback& callback,
    ErrorType error,
    const BackgroundSyncRegistration& sync_registration) {
  // The callback might delete this object, so hang onto a weak ptr to find out.
  base::WeakPtr<BackgroundSyncManager> manager = weak_ptr_factory_.GetWeakPtr();
  callback.Run(error, sync_registration);
  if (manager)
    op_scheduler_.CompleteOperationAndRunNext();
}

void BackgroundSyncManager::PendingStatusCallback(
    const StatusCallback& callback,
    ErrorType error) {
  // The callback might delete this object, so hang onto a weak ptr to find out.
  base::WeakPtr<BackgroundSyncManager> manager = weak_ptr_factory_.GetWeakPtr();
  callback.Run(error);
  if (manager)
    op_scheduler_.CompleteOperationAndRunNext();
}

void BackgroundSyncManager::PendingClosure(const base::Closure& callback) {
  // The callback might delete this object, so hang onto a weak ptr to find out.
  base::WeakPtr<BackgroundSyncManager> manager = weak_ptr_factory_.GetWeakPtr();
  callback.Run();
  if (manager)
    op_scheduler_.CompleteOperationAndRunNext();
}

base::Closure BackgroundSyncManager::MakeEmptyCompletion() {
  return base::Bind(&BackgroundSyncManager::PendingClosure,
                    weak_ptr_factory_.GetWeakPtr(),
                    base::Bind(base::DoNothing));
}

BackgroundSyncManager::StatusAndRegistrationCallback
BackgroundSyncManager::MakeStatusAndRegistrationCompletion(
    const StatusAndRegistrationCallback& callback) {
  return base::Bind(
      &BackgroundSyncManager::PendingStatusAndRegistrationCallback,
      weak_ptr_factory_.GetWeakPtr(), callback);
}

BackgroundSyncManager::StatusCallback
BackgroundSyncManager::MakeStatusCompletion(const StatusCallback& callback) {
  return base::Bind(&BackgroundSyncManager::PendingStatusCallback,
                    weak_ptr_factory_.GetWeakPtr(), callback);
}

}  // namespace content
