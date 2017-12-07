// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_PAGE_MODEL_TYPES_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_PAGE_MODEL_TYPES_H_

namespace offline_pages {

// Result for synchronous operations (like database and file operations) that
// are part of the tasks used by Offline Pages.
enum class SyncOperationResult {
  SUCCESS,                   // Successful operation
  INVALID_DB_CONNECTION,     // Invalid database connection
  TRANSACTION_BEGIN_ERROR,   // Failed when start a DB transaction
  TRANSACTION_COMMIT_ERROR,  // Failed when commiting a DB transaction
  DB_OPERATION_ERROR,        // Failed when executing a DB statement
  FILE_OPERATION_ERROR,      // Failed while doing file operations
  // NOTE: always keep this entry at the end. Add new result types only
  // immediately above this line. Make sure to update the corresponding
  // histogram enum accordingly.
  RESULT_COUNT,
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_OFFLINE_PAGE_MODEL_TYPES_H_
