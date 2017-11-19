// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_ACTIVATE_NEXT_PENDING_REQUEST_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_ACTIVATE_NEXT_PENDING_REQUEST_TASK_H_

#include <string>
#include <vector>

#include "base/guid.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/browser/background_fetch/background_fetch_request_manager.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

class ActivateNextPendingRequestTask : DatabaseTask {
 public:
  ActivateNextPendingRequestTask(
      BackgroundFetchDataManager* data_manager,
      const BackgroundFetchRegistrationId& registration_id,
      BackgroundFetchRequestManager::NextRequestCallback callback);

  ~ActivateNextPendingRequestTask() override;

  void Start() override;

 private:
  void DidGetNextPendingRequestKey(
      int64_t service_worker_registration_id,
      const flat_map<std::string, std::string>& pending_request_map,
      ServiceWorkerStatusCode status);

  void DidGetRegistrationAndRequest(
      int64_t service_worker_registration_id,
      const std::string& unique_id,
      int request_index,
      const std::string& pending_request_key,
      const std::vector<std::string>& registration_and_request,
      ServiceWorkerStatusCode status);

  void DidMarkRequestActive(DatabaseStatus status,
                            int64_t service_worker_registration_id,
                            const std::string& unique_id,
                            int request_index,
                            const std::string& download_guid,
                            std::string serialized_registration,
                            std::string serialized_request);

  BackgroundFetchRegistrationId registration_id_;

  BackgroundFetchRequestManager::NextRequestCallback callback_;

  base::WeakPtrFactory<ActivateNextPendingRequestTask>
      weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(ActivateNextPendingRequestTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_ACTIVATE_NEXT_PENDING_REQUEST_TASK_H_
