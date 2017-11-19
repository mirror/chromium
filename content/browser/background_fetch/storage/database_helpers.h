// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_HELPERS_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_HELPERS_H_

#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

// The database schema is content/browser/background_fetch/storage/README.md.
// When making any changes to these keys or the related functions, you must
// update the README.md file as well.

// Warning: registration |developer_id|s may contain kSeparator characters.
const char kSeparator[] = "_";

// Constants used for Service Worker UserData database keys.
const char kActiveRegistrationUniqueIdKeyPrefix[] =
    "bgfetch_active_registration_unique_id_";
const char kRegistrationKeyPrefix[] = "bgfetch_registration_";
const char kRequestKeyPrefix[] = "bgfetch_request_";
const char kPendingRequestKeyPrefix[] = "bgfetch_pending_request_";
const char kActiveRequestKeyPrefix[] = "bgfetch_active_request_";

std::string ActiveRegistrationUniqueIdKey(const std::string& developer_id);

std::string RegistrationKey(const std::string& unique_id);

std::string RequestKeyPrefix(const std::string& unique_id);

std::string PendingRequestKeyPrefix(const std::string& unique_id);

std::string PendingRequestKey(const std::string& unique_id, int request_index);

bool ParsePendingRequestKey(base::StringPiece pending_request_key,
                            std::string* unique_id,
                            int* request_index);

enum class DatabaseStatus { kOk, kFailed, kNotFound };

DatabaseStatus ToDatabaseStatus(ServiceWorkerStatusCode status);

#if DCHECK_IS_ON()
// Checks that a registration is or is not active. Use this as the callback to
// |GetRegistrationUserData(..., {ActiveRegistrationUniqueIdKey(...)}, ...)|.
void DCheckRegistrationActive(bool should_be_active,
                              const std::string& unique_id,
                              const std::vector<std::string>& data,
                              ServiceWorkerStatusCode status);
#endif  // DCHECK_IS_ON()

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_HELPERS_H_
