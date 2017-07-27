// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_UTILS_H_

#include <stdint.h>

namespace sql {
class Connection;
class Statement;
}  // namespace sql

namespace offline_pages {

struct PrefetchItem;

// Creates an offline ID for the prefetch item.
int64_t GenerateOfflineId();

// Deletes a prefetch item by its offline ID. Returns whether it was the item
// was successfully deleted.
bool DeletePrefetchItemByOfflineIdSync(sql::Connection* db, int64_t offline_id);

// Creates a prefetch item from a SQL result. Expects complete rows with all
// columns present.
void MakePrefetchItem(const sql::Statement& statement, PrefetchItem* item);

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_UTILS_H_
