// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_UTILS_H_

#include <stdint.h>

#include <memory>

#include "components/offline_pages/core/offline_store_types.h"

namespace sql {
class Connection;
class Statement;
}  // namespace sql

namespace offline_pages {
struct PrefetchItem;

typedef StoreUpdateResult<PrefetchItem> PrefetchItemsUpdateResult;

// Creates an offline ID for the prefetch item.
int64_t GenerateOfflineId();

// Converts the current row of the passed in statement to a prefetch item.
void MakePrefetchItem(sql::Statement* statement, PrefetchItem* item);

// Gets a single prefetch item by its offline ID. Returns true of such item
// exists, and false otherwise.
bool GetPrefetchItemByOfflineIdSync(sql::Connection* db,
                                    int64_t offline_id,
                                    PrefetchItem* item);

// Deletes a prefetch item by its offline ID. Returns whether it was the item
// was successfully deleted.
bool DeletePrefetchItemByOfflineIdSync(sql::Connection* db, int64_t offline_id);

// Build a result for store. This would typically be used to construct an error
// result.
std::unique_ptr<StoreUpdateResult<PrefetchItem>> BuildResultForIds(
    StoreState store_state,
    const std::vector<int64_t>& offline_ids,
    ItemActionStatus action_status);

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SQL_UTILS_H_
