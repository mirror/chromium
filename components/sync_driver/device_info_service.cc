// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_driver/device_info_service.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "sync/api/metadata_batch.h"
#include "sync/api/sync_error.h"
#include "sync/protocol/data_type_state.pb.h"
#include "sync/protocol/sync.pb.h"
#include "sync/util/time.h"

namespace sync_driver_v2 {

using syncer::SyncError;
using syncer_v2::EntityChangeList;
using syncer_v2::EntityData;
using syncer_v2::EntityDataList;
using syncer_v2::MetadataBatch;
using syncer_v2::MetadataChangeList;
using syncer_v2::ModelTypeStore;
using syncer_v2::SimpleMetadataChangeList;
using sync_driver::DeviceInfo;
using sync_pb::DeviceInfoSpecifics;

using Record = ModelTypeStore::Record;
using RecordList = ModelTypeStore::RecordList;
using Result = ModelTypeStore::Result;

DeviceInfoService::DeviceInfoService(
    sync_driver::LocalDeviceInfoProvider* local_device_info_provider,
    const StoreFactoryFunction& callback)
    : local_device_backup_time_(-1),
      local_device_info_provider_(local_device_info_provider),
      weak_factory_(this) {
  DCHECK(local_device_info_provider);

  // This is not threadsafe, but presuably the provider initializes on the same
  // thread as us so we're okay.
  if (local_device_info_provider->GetLocalDeviceInfo()) {
    OnProviderInitialized();
  } else {
    subscription_ =
        local_device_info_provider->RegisterOnInitializedCallback(base::Bind(
            &DeviceInfoService::OnProviderInitialized, base::Unretained(this)));
  }

  callback.Run(base::Bind(&DeviceInfoService::OnStoreCreated,
                          weak_factory_.GetWeakPtr()));
}

DeviceInfoService::~DeviceInfoService() {}

scoped_ptr<MetadataChangeList> DeviceInfoService::CreateMetadataChangeList() {
  return make_scoped_ptr(new SimpleMetadataChangeList());
}

SyncError DeviceInfoService::MergeSyncData(
    scoped_ptr<MetadataChangeList> metadata_change_list,
    EntityDataList entity_data_list) {
  // TODO(skym): crbug.com/543406: Implementation.
  return SyncError();
}

SyncError DeviceInfoService::ApplySyncChanges(
    scoped_ptr<MetadataChangeList> metadata_change_list,
    EntityChangeList entity_changes) {
  // TODO(skym): crbug.com/543406: Implementation.
  return SyncError();
}

void DeviceInfoService::GetData(ClientTagList client_tags,
                                DataCallback callback) {
  // TODO(skym): crbug.com/543405: Implementation.
}

void DeviceInfoService::GetAllData(DataCallback callback) {
  // TODO(skym): crbug.com/543405: Implementation.
}

std::string DeviceInfoService::GetClientTag(const EntityData& entity_data) {
  DCHECK(entity_data.specifics.has_device_info());
  return entity_data.specifics.device_info().cache_guid();
}

void DeviceInfoService::OnChangeProcessorSet() {
  TryLoadAllMetadata();
}

bool DeviceInfoService::IsSyncing() const {
  return !all_data_.empty();
}

scoped_ptr<DeviceInfo> DeviceInfoService::GetDeviceInfo(
    const std::string& client_id) const {
  ClientIdToSpecifics::const_iterator iter = all_data_.find(client_id);
  if (iter == all_data_.end()) {
    return scoped_ptr<DeviceInfo>();
  }

  return CreateDeviceInfo(*iter->second);
}

ScopedVector<DeviceInfo> DeviceInfoService::GetAllDeviceInfo() const {
  ScopedVector<DeviceInfo> list;

  for (ClientIdToSpecifics::const_iterator iter = all_data_.begin();
       iter != all_data_.end(); ++iter) {
    list.push_back(CreateDeviceInfo(*iter->second));
  }

  return list;
}

void DeviceInfoService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DeviceInfoService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void DeviceInfoService::NotifyObservers() {
  FOR_EACH_OBSERVER(Observer, observers_, OnDeviceInfoChange());
}

void DeviceInfoService::UpdateLocalDeviceBackupTime(base::Time backup_time) {
  // TODO(skym): crbug.com/582460: Replace with is initialized check, we've
  // already started syncing, provider is ready, make sure we have processor.

  // Local device info must be available in advance.
  DCHECK(local_device_info_provider_->GetLocalDeviceInfo());

  // TODO(skym): crbug.com/582460: Less than instead of not equal check?
  if (GetLocalDeviceBackupTime() != backup_time) {
    // TODO(skym): crbug.com/582460: Storing this field doesn't really make
    // sense.
    set_local_device_backup_time(syncer::TimeToProtoTime(backup_time));
    scoped_ptr<DeviceInfoSpecifics> new_specifics = CreateLocalSpecifics();

    // TODO(skym): crbug.com/543406: Create EntityChange and pass to SMTP
    // through ProcessChanges.
    // TODO(skym): crbug.com/543405: Persist metadata and data.
    StoreSpecifics(std::move(new_specifics));
  }

  // Don't call NotifyObservers() because backup time is not part of
  // DeviceInfoTracker interface.
}

base::Time DeviceInfoService::GetLocalDeviceBackupTime() const {
  return has_local_device_backup_time()
             ? syncer::ProtoTimeToTime(local_device_backup_time())
             : base::Time();
}

// TODO(skym): crbug.com/543406: It might not make sense for this to be a
// scoped_ptr.
scoped_ptr<DeviceInfoSpecifics> DeviceInfoService::CreateLocalSpecifics() {
  const DeviceInfo* info = local_device_info_provider_->GetLocalDeviceInfo();
  DCHECK(info);
  scoped_ptr<DeviceInfoSpecifics> specifics = CreateSpecifics(*info);
  if (has_local_device_backup_time()) {
    specifics->set_backup_timestamp(local_device_backup_time());
  }
  // TODO(skym): crbug.com:543406: Local tag and non unique name have no place
  // to be set now.
  return specifics;
}

// TODO(skym): crbug.com/543406: It might not make sense for this to be a
// scoped_ptr.
// Static.
scoped_ptr<DeviceInfoSpecifics> DeviceInfoService::CreateSpecifics(
    const DeviceInfo& info) {
  scoped_ptr<DeviceInfoSpecifics> specifics =
      make_scoped_ptr(new DeviceInfoSpecifics);
  specifics->set_cache_guid(info.guid());
  specifics->set_client_name(info.client_name());
  specifics->set_chrome_version(info.chrome_version());
  specifics->set_sync_user_agent(info.sync_user_agent());
  specifics->set_device_type(info.device_type());
  specifics->set_signin_scoped_device_id(info.signin_scoped_device_id());
  return specifics;
}

// Static.
scoped_ptr<DeviceInfo> DeviceInfoService::CreateDeviceInfo(
    const DeviceInfoSpecifics& specifics) {
  return make_scoped_ptr(new DeviceInfo(
      specifics.cache_guid(), specifics.client_name(),
      specifics.chrome_version(), specifics.sync_user_agent(),
      specifics.device_type(), specifics.signin_scoped_device_id()));
}

void DeviceInfoService::StoreSpecifics(
    scoped_ptr<DeviceInfoSpecifics> specifics) {
  DVLOG(1) << "Storing DEVICE_INFO for " << specifics->client_name()
           << " with ID " << specifics->cache_guid();
  all_data_[specifics->cache_guid()] = std::move(specifics);
}

void DeviceInfoService::DeleteSpecifics(const std::string& client_id) {
  ClientIdToSpecifics::const_iterator iter = all_data_.find(client_id);
  if (iter != all_data_.end()) {
    DVLOG(1) << "Deleting DEVICE_INFO for " << iter->second->client_name()
             << " with ID " << client_id;
    all_data_.erase(iter);
  }
}

void DeviceInfoService::OnProviderInitialized() {
  has_provider_initialized_ = true;
  TryReconcileLocalAndStored();
}

void DeviceInfoService::OnStoreCreated(Result result,
                                       scoped_ptr<ModelTypeStore> store) {
  if (result == Result::SUCCESS) {
    std::swap(store_, store);
    store_->ReadAllData(base::Bind(&DeviceInfoService::OnReadAllData,
                                   weak_factory_.GetWeakPtr()));
  } else {
    LOG(WARNING) << "ModelTypeStore creation failed.";
    // TODO(skym, crbug.com/582460): Handle unrecoverable initialization
    // failure.
  }
}

void DeviceInfoService::OnReadAllData(Result result,
                                      scoped_ptr<RecordList> record_list) {
  if (result == Result::SUCCESS) {
    for (const Record& r : *record_list.get()) {
      scoped_ptr<DeviceInfoSpecifics> specifics(
          make_scoped_ptr(new DeviceInfoSpecifics()));
      if (specifics->ParseFromString(r.value)) {
        all_data_[r.id] = std::move(specifics);
      } else {
        LOG(WARNING) << "Failed to deserialize specifics.";
        // TODO(skym, crbug.com/582460): Handle unrecoverable initialization
        // failure.
      }
    }
    has_data_loaded_ = true;
    TryLoadAllMetadata();
  } else {
    LOG(WARNING) << "Initial load of data failed.";
    // TODO(skym, crbug.com/582460): Handle unrecoverable initialization
    // failure.
  }
}

void DeviceInfoService::OnReadAllMetadata(
    Result result,
    scoped_ptr<RecordList> metadata_records,
    const std::string& global_metadata) {
  if (!change_processor()) {
    // This datatype was disabled while this read was oustanding.
    return;
  }
  if (result != Result::SUCCESS) {
    // Store has encountered some serious error. We should still be able to
    // continue as a read only service, since if we got this far we must have
    // loaded all data out succesfully. TODO(skym): Should we communicate this
    // to sync somehow?
    LOG(WARNING) << "Load of metadata completely failed.";
    return;
  }
  scoped_ptr<MetadataBatch> batch(new MetadataBatch());
  sync_pb::DataTypeState state;
  if (state.ParseFromString(global_metadata)) {
    batch->SetDataTypeState(state);
  } else {
    // TODO(skym): How bad is this scenario? We may be able to just give an
    // empty batch to the processor and we'll treat corrupted data type state
    // as no data type state at all. The question is do we want to add any of
    // the entity metadata to the batch or completely skip that step? We're
    // going to have to perform a merge shortly. Does this decision/logic even
    // belong in this service?
    LOG(WARNING) << "Failed to deserialize global metadata.";
  }
  for (const Record& r : *metadata_records.get()) {
    sync_pb::EntityMetadata entity_metadata;
    if (entity_metadata.ParseFromString(r.value)) {
      batch->AddMetadata(r.id, entity_metadata);
    } else {
      // TODO(skym): This really isn't too bad. We just want to regenerate
      // metadata for this particular entity. Unfortunately there isn't a
      // convinient way to tell the processor to do this.
      LOG(WARNING) << "Failed to deserialize entity metadata.";
    }
  }
  change_processor()->OnMetadataLoaded(std::move(batch));
}

void DeviceInfoService::TryReconcileLocalAndStored() {
  // TODO(skym, crbug.com/582460): Implement logic to reconcile provider and
  // stored device infos.
}

void DeviceInfoService::TryLoadAllMetadata() {
  if (has_data_loaded_ && change_processor()) {
    store_->ReadAllMetadata(base::Bind(&DeviceInfoService::OnReadAllMetadata,
                                       weak_factory_.GetWeakPtr()));
  }
}

}  // namespace sync_driver_v2
