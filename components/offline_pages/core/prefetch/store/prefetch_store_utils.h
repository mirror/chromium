// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_UTILS_H_

#include <stdint.h>

namespace sql {
class Connection;
}  // namespace sql

namespace offline_pages {

// Creates an offline ID for the prefetch item.
int64_t GenerateOfflineId();

// Checks whether the |db| points to a valid and opened database. Returns true
// if that is the case, false otherwise. Should be used at the beginning of
// command implementation, before transaction begins.
bool CheckDb(sql::Connection* db);

// Deletes a prefetch item by its offline ID. Returns whether it was the item
// was successfully deleted.
bool DeletePrefetchItemByOfflineIdSync(sql::Connection* db, int64_t offline_id);

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_UTILS_H_
