// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_HELPERS_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_HELPERS_H_

#include <string>

#include "content/common/service_worker/service_worker_status_code.h"

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

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DATABASE_HELPERS_H_
