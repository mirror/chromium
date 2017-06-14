// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/delete_prefetch_items_command_sql.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

typedef base::Callback<void(std::unique_ptr<StoreUpdateResult<PrefetchItem>>)>
    UpdateCallback;

void RemovePrefetchItemsSync(sql::Connection* db,
                             const std::vector<int64_t>& offline_ids,
                             scoped_refptr<base::SingleThreadTaskRunner> runner,
                             const UpdateCallback& callback) {
  std::unique_ptr<StoreUpdateResult<PrefetchItem>> result(
      new StoreUpdateResult<PrefetchItem>(StoreState::LOADED));

  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    // Extract appropriate store function
    std::unique_ptr<PrefetchItemsUpdateResult> result = BuildResultForIds(
        StoreState::LOADED, offline_ids, ItemActionStatus::STORE_ERROR);
    runner->PostTask(FROM_HERE, base::Bind(callback, base::Passed(&result)));
    return;
  }

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
    std::unique_ptr<PrefetchItemsUpdateResult> result = BuildResultForIds(
        StoreState::LOADED, offline_ids, ItemActionStatus::STORE_ERROR);
    runner->PostTask(FROM_HERE, base::Bind(callback, base::Passed(&result)));
    return;
  }

  runner->PostTask(FROM_HERE, base::Bind(callback, base::Passed(&result)));
}
}  // namespace

DeletePrefetchItemsCommandSql::DeletePrefetchItemsCommandSql(
    const std::vector<int64_t>& offline_ids)
    : offline_ids_(offline_ids), weak_ptr_factory_(this) {}

DeletePrefetchItemsCommandSql::~DeletePrefetchItemsCommandSql() {}

void DeletePrefetchItemsCommandSql::Execute(
    sql::Connection* db,
    scoped_refptr<base::SingleThreadTaskRunner> background_runner) {
  background_runner->PostTask(
      FROM_HERE,
      base::Bind(
          &RemovePrefetchItemsSync, db, offline_ids_,
          base::ThreadTaskRunnerHandle::Get(),
          base::Bind(
              &DeletePrefetchItemsCommandSql::RemovePrefetchItemsSyncDone,
              weak_ptr_factory_.GetWeakPtr())));
}

void DeletePrefetchItemsCommandSql::RemovePrefetchItemsSyncDone(
    std::unique_ptr<StoreUpdateResult<PrefetchItem>> result) {}

}  // namespace offline_pages
