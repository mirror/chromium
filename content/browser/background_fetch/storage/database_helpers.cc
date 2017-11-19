// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/database_helpers.h"

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "content/browser/background_fetch/background_fetch.pb.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

namespace background_fetch {

std::string ActiveRegistrationUniqueIdKey(const std::string& developer_id) {
  // Allows looking up the active registration's |unique_id| by |developer_id|.
  // Registrations are active from creation up until completed/failed/aborted.
  // These database entries correspond to the active background fetches map:
  // https://wicg.github.io/background-fetch/#service-worker-registration-active-background-fetches
  return kActiveRegistrationUniqueIdKeyPrefix + developer_id;
}

std::string RegistrationKey(const std::string& unique_id) {
  // Allows looking up a registration by |unique_id|.
  return kRegistrationKeyPrefix + unique_id;
}

std::string RequestKeyPrefix(const std::string& unique_id) {
  // Allows looking up all requests within a registration.
  return kRequestKeyPrefix + unique_id + kSeparator;
}

std::string PendingRequestKeyPrefix(const std::string& unique_id) {
  // TODO(crbug.com/741609): These keys should be ordered by the registration's
  // creation time (or a more advanced ordering) rather than by its |unique_id|,
  // so that the highest priority pending requests in FIFO order can be looked
  // up by fetching the lexicographically smallest keys. However the keys are
  // temporarily ordered by |unique_id| instead, since globally ordering request
  // keys requires refactoring requests to be globally scheduled, rather than
  // the current system where each JobController pulls requests one at a time.
  return kPendingRequestKeyPrefix + unique_id + kSeparator;
}

std::string PendingRequestKey(
    const std::string& unique_id,
    int request_index) {
  // In addition to the ordering from PendingRequestKeyPrefix, the requests
  // within each registration should be prioritized according to their index.

  // TODO(delphick): request_index must be padded or we will have 10<2.
  return PendingRequestKeyPrefix(unique_id) + base::IntToString(request_index);
}

bool ParsePendingRequestKey(base::StringPiece pending_request_key,
                            std::string* unique_id,
                            int* request_index) {
  DCHECK(unique_id);
  DCHECK(request_index);

  if (!pending_request_key.starts_with(kPendingRequestKeyPrefix))
    return false;

  pending_request_key.remove_prefix(strlen(kPendingRequestKeyPrefix));

  std::vector<base::StringPiece> parts =
      base::SplitStringPiece(pending_request_key, kSeparator,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (parts.size() != 2)
    return false;

  if (!base::IsValidGUIDOutputString(parts[0]))
    return false;

  int parsed_request_index;
  if (!base::StringToInt(parts[1], &parsed_request_index))
    return false;

  parts[0].CopyToString(unique_id);
  *request_index = parsed_request_index;
  return true;
}

DatabaseStatus ToDatabaseStatus(ServiceWorkerStatusCode status) {
  switch (status) {
    case SERVICE_WORKER_OK:
      return DatabaseStatus::kOk;
    case SERVICE_WORKER_ERROR_FAILED:
    case SERVICE_WORKER_ERROR_ABORT:
      // FAILED is for invalid arguments (e.g. empty key) or database errors.
      // ABORT is for unexpected failures, e.g. because shutdown is in progress.
      // BackgroundFetchDataManager handles both of these the same way.
      return DatabaseStatus::kFailed;
    case SERVICE_WORKER_ERROR_NOT_FOUND:
      // This can also happen for writes, if the ServiceWorkerRegistration has
      // been deleted.
      return DatabaseStatus::kNotFound;
    case SERVICE_WORKER_ERROR_START_WORKER_FAILED:
    case SERVICE_WORKER_ERROR_PROCESS_NOT_FOUND:
    case SERVICE_WORKER_ERROR_EXISTS:
    case SERVICE_WORKER_ERROR_INSTALL_WORKER_FAILED:
    case SERVICE_WORKER_ERROR_ACTIVATE_WORKER_FAILED:
    case SERVICE_WORKER_ERROR_IPC_FAILED:
    case SERVICE_WORKER_ERROR_NETWORK:
    case SERVICE_WORKER_ERROR_SECURITY:
    case SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED:
    case SERVICE_WORKER_ERROR_STATE:
    case SERVICE_WORKER_ERROR_TIMEOUT:
    case SERVICE_WORKER_ERROR_SCRIPT_EVALUATE_FAILED:
    case SERVICE_WORKER_ERROR_DISK_CACHE:
    case SERVICE_WORKER_ERROR_REDUNDANT:
    case SERVICE_WORKER_ERROR_DISALLOWED:
    case SERVICE_WORKER_ERROR_MAX_VALUE:
      break;
  }
  NOTREACHED();
  return DatabaseStatus::kFailed;
}

#if DCHECK_IS_ON()
void DCheckRegistrationActive(bool should_be_active,
                              const std::string& unique_id,
                              const std::vector<std::string>& data,
                              ServiceWorkerStatusCode status) {
  // TODO(crbug.com/780027): Nuke the database if any DCHECK would fail.
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
      DCHECK_EQ(1u, data.size());
      if (should_be_active)
        DCHECK_EQ(unique_id, data[0]);
      else
        DCHECK_NE(unique_id, data[0]);
      return;
    case DatabaseStatus::kFailed:
      // TODO(crbug.com/780025): Consider logging failure to UMA.
      return;
    case DatabaseStatus::kNotFound:
      if (should_be_active)
        NOTREACHED();
      return;
  }
}
#endif  // DCHECK_IS_ON()

}  // namespace background_fetch

}  // namespace content
