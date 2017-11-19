// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/activate_next_pending_request_task.h"

#include <utility>

namespace content {

namespace background_fetch {

ActivateNextPendingRequestTask::ActivateNextPendingRequestTask(
    BackgroundFetchDataManager* data_manager,
    const BackgroundFetchRegistrationId& registration_id,
    BackgroundFetchRequestManager::NextRequestCallback callback)
    : DatabaseTask(data_manager),
      registration_id_(registration_id),
      callback_(std::move(callback)),
      weak_factory_(this) {}

ActivateNextPendingRequestTask::~ActivateNextPendingRequestTask() = default;

void ActivateNextPendingRequestTask::Start() {
  service_worker_context()->GetRegistrationUserKeysAndDataByKeyPrefix(
      registration_id_.service_worker_registration_id_,
      PendingRequestKeyPrefix(registration_id_.unique_id()),
      base::Bind(&ActivateNextPendingRequestTask::DidGetNextPendingRequestKey,
                 weak_factory_.GetWeakPtr()),
      1 /* retrieve just a single request */);
}

void ActivateNextPendingRequestTask::DidGetNextPendingRequestKey(
    int64_t service_worker_registration_id,
    const flat_map<std::string, std::string>& pending_request_map,
    ServiceWorkerStatusCode status) {
  if (ToDatabaseStatus(status) != DatabaseStatus::kOk ||
      pending_request_map.size() != 1) {
    // TODO(crbug.com/780025): Log failure to UMA.
    std::move(callback_).Run(nullptr /* request */);
    Finished();  // Destroys |this|.
    return;
  }

  // Since the prefix was bgfetch_pending_request_<unique_id>_, the single
  // returned key will be be the request number.
  const std::string& pending_request_key = pending_request_map.begin()->first;
  int request_index;
  if (!base::StringToInt(pending_request_key, &request_index)) {
    // TODO(crbug.com/780027): Nuke it.
    NOTREACHED() << "Database is corrupt";
    return;
  }

  // Get corresponding serialized registration and serialized request.
  service_worker_context()->GetRegistrationUserData(
      service_worker_registration_id,
      {RegistrationKey(registration_id_.unique_id()),
       RequestKey(unique_id, request_index)},
      base::Bind(&ActivateNextPendingRequestTask::DidGetRegistrationAndRequest,
                 weak_factory_.GetWeakPtr(), service_worker_registration_id,
                 unique_id, request_index, pending_request_key));

  service_worker_context()->ClearAndStoreRegistrationUserData(
      service_worker_registration_id, base::nullopt /* origin */,
      {pending_request_key},
      {{ActiveRequestKey(unique_id, request_index), download_guid}},
      base::Bind(&ActivateNextPendingRequestTask::DidMarkRequestActive,
                 weak_factory_.GetWeakPtr(), service_worker_registration_id,
                 unique_id, request_index, download_guid,
                 std::move(serialized_registration),
                 std::move(serialized_request)));
}

void ActivateNextPendingRequestTask::DidGetRegistrationAndRequest(
    int64_t service_worker_registration_id,
    const std::string& unique_id,
    int request_index,
    const std::string& pending_request_key,
    const std::vector<std::string>& registration_and_request,
    ServiceWorkerStatusCode status) {
  if (ToDatabaseStatus(status) != DatabaseStatus::kOk) {
    // TODO(crbug.com/780025): Log failure to UMA.
    std::move(callback_).Run(nullptr /* request */);
    Finished();  // Destroys |this|.
    return;
  }

  DCHECK_EQ(2u, registration_and_request.size());
  std::string serialized_registration = registration_and_request[0];
  std::string serialized_request = registration_and_request[1];

  // Generate a fresh |download_guid| for the Download Service to use to
  // identify the activating request.
  std::string download_guid = base::GenerateGUID();

  // Update request state from pending to active.
  service_worker_context()->ClearAndStoreRegistrationUserData(
      service_worker_registration_id, base::nullopt /* origin */,
      {pending_request_key},
      {{ActiveRequestKey(unique_id, request_index), download_guid}},
      base::Bind(&ActivateNextPendingRequestTask::DidMarkRequestActive,
                 weak_factory_.GetWeakPtr(), service_worker_registration_id,
                 unique_id, request_index, download_guid,
                 std::move(serialized_registration),
                 std::move(serialized_request)));
}

void ActivateNextPendingRequestTask::DidMarkRequestActive(
    DatabaseStatus status,
    int64_t service_worker_registration_id,
    const std::string& unique_id,
    int request_index,
    const std::string& download_guid,
    std::string serialized_registration,
    std::string serialized_request) {
  if (ToDatabaseStatus(status) != DatabaseStatus::kOk) {
    // TODO(crbug.com/780025): Log failure to UMA.
    std::move(callback_).Run(nullptr /* request */);
    Finished();  // Destroys |this|.
    return;
  }

  // TODO(crbug.com/741609): Once requests are globally scheduled, the
  // Scheduler will be calling ActivateNextPendingRequest instead of the
  // JobController, and this will have to extract BackgroundFetchOptions etc
  // from the |registration_proto| and pass them to |callback_|, so that the
  // Scheduler can create a JobController for that registration if necessary.
  proto::BackgroundFetchRegistration registration_proto;
  if (!registration_proto.ParseFromString(serialized_registration) ||
      !registration_proto.has_unique_id() ||
      !registration_proto.has_developer_id() ||
      !registration_proto.has_origin() ||
      !registration_proto.has_creation_microseconds_since_unix_epoch()) {
    // TODO(crbug.com/780027): Nuke it.
    NOTREACHED() << "Database is corrupt";
    return;
  }

#if DCHECK_IS_ON()
  // Only active registrations should have pending requests.
  service_worker_context()->GetRegistrationUserData(
      service_worker_registration_id_,
      {ActiveRegistrationUniqueIdKey(registration_proto.developer_id())},
      base::Bind(&DCheckRegistrationActive, true /* should_be_active */,
                 unique_id_));
#endif  // DCHECK_IS_ON()

  proto::BackgroundFetchRequest request_proto;
  if (!request_proto.ParseFromString(serialized_request) ||
      !request_proto.has_foo() || !request_proto.has_bar() ||
      !request_proto.has_baz()) {
    // TODO(crbug.com/780027): Nuke it.
    NOTREACHED() << "Database is corrupt";
    return;
  }

  // Deserialize request.
  scoped_refptr<BackgroundFetchRequestInfo> request;
  if (status == DatabaseStatus::kOk &&
      !DeserializeRequest(serialized_request, download_guid, &request)) {
    status = DatabaseStatus::kFailed;
  }

  std::move(callback_).Run(std::move(request));
  Finished();  // Destroys |this|.
}

}  // namespace background_fetch

}  // namespace content
