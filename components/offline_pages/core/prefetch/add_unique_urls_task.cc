// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/add_unique_urls_task.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

// Finds offline IDs of prefetch items with a given namespace and state.
std::unordered_set<int64_t> FindItemIdsWithNamespaceAndState(
    sql::Connection* db,
    const std::string& name_space,
    PrefetchItemState state) {
  const char kSql[] =
      "SELECT offline_id FROM prefetch_items"
      " WHERE client_namespace = ? AND state = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, name_space);
  statement.BindInt(1, static_cast<int>(state));

  std::unordered_set<int64_t> result;
  while (statement.Step())
    result.insert(statement.ColumnInt64(0));

  return result;
}

std::unordered_map<std::string, int64_t> FindRequestedUrlsInNamespace(
    sql::Connection* db,
    const std::string& name_space) {
  const char kSql[] =
      "SELECT requested_url, offline_id FROM prefetch_items"
      " WHERE client_namespace = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, name_space);

  std::unordered_map<std::string, int64_t> result;
  while (statement.Step())
    result.insert(
        std::make_pair(statement.ColumnString(0), statement.ColumnInt64(1)));

  return result;
}

ItemActionStatus CreatePrefetchItem(sql::Connection* db,
                                    const std::string& name_space,
                                    const PrefetchURL& prefetch_url) {
  const char kSql[] =
      "INSERT OR IGNORE INTO prefetch_items"
      " (offline_id, requested_url, client_namespace, client_id)"
      " VALUES"
      " (?, ?, ?, ?)";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, GenerateOfflineId());
  statement.BindString(1, prefetch_url.url.spec());
  statement.BindString(2, name_space);
  statement.BindString(3, prefetch_url.id);

  if (!statement.Run())
    return ItemActionStatus::STORE_ERROR;
  // Rather unexpected outcome.
  if (db->GetLastChangeCount() == 0)
    return ItemActionStatus::ALREADY_EXISTS;
  return ItemActionStatus::ITEM_ADDED;
}

// Adds new prefetch item entries to the store using the URLs and client IDs
// from |prefetch_urls| and the client's |name_space|. Also cleans up entries in
// the Zombie state from the client's |name_space| except for the ones
// whose URL is contained in |prefetch_urls|.
// Returns the number of added prefecth items.
static int AddUrlsAndCleanupZombiesSync(
    const std::string& name_space,
    const std::vector<PrefetchURL>& prefetch_urls,
    sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return -1;

  std::unordered_set<int64_t> zombie_ids = FindItemIdsWithNamespaceAndState(
      db, name_space, PrefetchItemState::ZOMBIE);
  LOG(ERROR) << "Zombie count: " << zombie_ids.size();

  std::unordered_map<std::string, int64_t> requested_urls_and_ids =
      FindRequestedUrlsInNamespace(db, name_space);
  LOG(ERROR) << "requested_urls_and_ids.size: "
             << requested_urls_and_ids.size();

  // TODO(fgorski): Populate result.
  for (auto prefetch_url : prefetch_urls) {
    // Default status presumes that there is already an entry for a given item.
    ItemActionStatus status = ItemActionStatus::ALREADY_EXISTS;

    auto iter = requested_urls_and_ids.find(prefetch_url.url.spec());
    if (iter == requested_urls_and_ids.end()) {
      status = CreatePrefetchItem(db, name_space, prefetch_url);
    } else if (zombie_ids.count(iter->second) > 0) {
      zombie_ids.erase(zombie_ids.find(iter->second));
    }
    // Ignoring the requested URL otherwise since it is in progress.
  }

  // Purge remaining zombie IDs.
  for (int64_t offline_id : zombie_ids) {
    if (DeletePrefetchItemByOfflineIdSync(db, offline_id)) {
      // Add ITEM_DELETED status to result.
      ;
    }
  }

  if (!transaction.Commit())
    return -1;

  return 1;
}

}

AddUniqueUrlsTask::AddUniqueUrlsTask(
    PrefetchStoreSQL* prefetch_store,
    const std::string& name_space,
    const std::vector<PrefetchURL>& prefetch_urls)
    : prefetch_store_(prefetch_store),
      name_space_(name_space),
      prefetch_urls_(prefetch_urls),
      weak_ptr_factory_(this) {}

AddUniqueUrlsTask::~AddUniqueUrlsTask() {}

void AddUniqueUrlsTask::Run() {
  prefetch_store_->Execute<int>(base::BindOnce(&AddUrlsAndCleanupZombiesSync,
                                               name_space_, prefetch_urls_),
                                base::BindOnce(&AddUniqueUrlsTask::OnUrlsAdded,
                                               weak_ptr_factory_.GetWeakPtr()));
}

void AddUniqueUrlsTask::OnUrlsAdded(int added_entry_count) {
  // TODO(carlosk): schedule NWake here if at least one new entry was added to
  // the store.
  TaskComplete();
}

}  // namespace offline_pages
