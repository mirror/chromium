// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_TASK_H_

#include <memory>
#include <set>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "third_party/WebKit/public/platform/modules/background_fetch/background_fetch.mojom.h"

// Service Worker DB UserData schema
// =================================
// Design doc:
// https://docs.google.com/document/d/1-WPPTP909Gb5PnaBOKP58tPVLw2Fq0Ln-u1EBviIBns/edit
//
// - Each key will be stored twice by the Service Worker DB, once as a
//   "REG_HAS_USER_DATA:", and once as a "REG_USER_DATA:" - see
//   content/browser/service_worker/service_worker_database.cc for details.
// - Integer values are serialized as a string by base::Int*ToString().
//
// key: "bgfetch_active_registration_unique_id_" + <std::string 'developer_id'>
// value: <std::string 'unique_id'>
//
// key: "bgfetch_registration_" + <std::string 'unique_id'>
// value: <std::string 'serialized content::proto::BackgroundFetchRegistration'>
//
// key: "bgfetch_request_" + <std::string 'registration unique_id'>
//          + "_" + <int 'request_index'>
// value: <TODO: FetchAPIRequest serialized as a string>
//
// key: "bgfetch_pending_request_"
//          + <int64_t 'registration_creation_microseconds_since_unix_epoch'>
//          + "_" + <std::string 'registration unique_id'>
//          + "_" + <int 'request_index'>
// value: ""

namespace content {

class BackgroundFetchDataManager;
class BackgroundFetchDatabaseClient;
class ServiceWorkerContextWrapper;

// Note that this also handles non-error cases where the NONE is NONE.
using HandleBackgroundFetchErrorCallback =
    base::OnceCallback<void(blink::mojom::BackgroundFetchError)>;

namespace background_fetch {

// Warning: registration |developer_id|s may contain kSeparator characters.
const char kSeparator[] = "_";

const char kRequestKeyPrefix[] = "bgfetch_request_";
const char kRegistrationKeyPrefix[] = "bgfetch_registration_";
const char kPendingRequestKeyPrefix[] = "bgfetch_pending_request_";
const char kActiveRegistrationUniqueIdKeyPrefix[] =
    "bgfetch_active_registration_unique_id_";

std::string ActiveRegistrationUniqueIdKey(const std::string& developer_id);

std::string RegistrationKey(const std::string& unique_id);

std::string RequestKeyPrefix(const std::string& unique_id);

std::string PendingRequestKeyPrefix(
    int64_t registration_creation_microseconds_since_unix_epoch,
    const std::string& unique_id);

std::string PendingRequestKey(
    int64_t registration_creation_microseconds_since_unix_epoch,
    const std::string& unique_id,
    int request_index);

enum class DatabaseStatus { kOk, kFailed, kNotFound };

DatabaseStatus ToDatabaseStatus(ServiceWorkerStatusCode status);

// A DatabaseTask is an asynchronous "transaction" that needs to read/write the
// Service Worker Database.
//
// Only one DatabaseTask can run at once per StoragePartition, and no other code
// reads/writes Background Fetch keys, so each task effectively has an exclusive
// lock, except that core Service Worker code may delete all keys for a
// ServiceWorkerRegistration or the entire database at any time.
class DatabaseTask {
 public:
  virtual ~DatabaseTask() = default;

  virtual void Start() = 0;

 protected:
  explicit DatabaseTask(BackgroundFetchDataManager* data_manager)
      : data_manager_(data_manager) {
    DCHECK(data_manager_);
  }

  // Each task MUST call this once finished, even if exceptions occur, to
  // release their lock and allow the next task to execute.
  void Finished();

  BackgroundFetchDatabaseClient* GetController(const std::string& unique_id);

  void AddDatabaseTask(std::unique_ptr<DatabaseTask> task);

  ServiceWorkerContextWrapper* service_worker_context();

  std::set<std::string>& ref_counted_unique_ids();

  BackgroundFetchDataManager* data_manager() { return data_manager_; }

 private:
  BackgroundFetchDataManager* data_manager_;  // Owns this.

  DISALLOW_COPY_AND_ASSIGN(DatabaseTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_TASK_H_
