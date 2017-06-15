// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/delete_prefetch_items_sql_command.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

std::unique_ptr<StoreUpdateResult<PrefetchItem>> RemovePrefetchItemsSync(
    const std::vector<int64_t>& offline_ids,
    sql::Connection* db) {
  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return BuildResultForIds(StoreState::LOADED, offline_ids,
                             ItemActionStatus::STORE_ERROR);
  }

  std::unique_ptr<StoreUpdateResult<PrefetchItem>> result(
      new StoreUpdateResult<PrefetchItem>(StoreState::LOADED));

  for (int64_t offline_id : offline_ids) {
    PrefetchItem item;
    ItemActionStatus status;
    if (!GetPrefetchItemByOfflineIdSync(db, offline_id, &item)) {
      status = ItemActionStatus::NOT_FOUND;
    } else if (!DeletePrefetchItemByOfflineIdSync(db, offline_id)) {
      status = ItemActionStatus::STORE_ERROR;
    } else {
      status = ItemActionStatus::SUCCESS;
      result->updated_items.push_back(item);
    }

    result->item_statuses.push_back(std::make_pair(offline_id, status));
  }

  if (!transaction.Commit()) {
    return BuildResultForIds(StoreState::LOADED, offline_ids,
                             ItemActionStatus::STORE_ERROR);
  }

  return result;
}
}  // namespace

DeletePrefetchItemsCommandSql::DeletePrefetchItemsCommandSql(
    SqlCommandExecutor* command_executor)
    : command_executor_(command_executor) {}

DeletePrefetchItemsCommandSql::~DeletePrefetchItemsCommandSql() {}

void DeletePrefetchItemsCommandSql::Execute(
    const std::vector<int64_t>& offline_ids,
    const ResultCallback& result_callback) {
  command_executor_->Execute(
      base::BindOnce(&RemovePrefetchItemsSync, offline_ids), result_callback);
}
}  // namespace offline_pages
