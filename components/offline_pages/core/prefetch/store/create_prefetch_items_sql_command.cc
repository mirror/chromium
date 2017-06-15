// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/create_prefetch_items_sql_command.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

std::unique_ptr<std::vector<ItemActionStatus>> CreatePrefetchItemsSync(
    const std::string& name_space,
    const std::vector<PrefetchURL>& urls,
    sql::Connection* db) {
  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return base::MakeUnique<std::vector<ItemActionStatus>>(
        urls.size(), ItemActionStatus::STORE_ERROR);
  }

  std::unique_ptr<std::vector<ItemActionStatus>> result(
      new std::vector<ItemActionStatus>);

  const char kSql[] =
      "INSERT OR IGNORE INTO prefetch_items"
      " (offline_id, requested_url, client_namespace, client_id)"
      " VALUES"
      " (?, ?, ?, ?)";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  for (PrefetchURL url : urls) {
    statement.BindInt64(0, /* generate valid offline id */ 0);
    statement.BindString(1, url.url.spec());
    statement.BindString(2, name_space);
    statement.BindString(3, url.client_id.id);
    ItemActionStatus status = ItemActionStatus::SUCCESS;
    if (!statement.Run())
      status = ItemActionStatus::STORE_ERROR;
    if (db->GetLastChangeCount() == 0)
      status = ItemActionStatus::ALREADY_EXISTS;
    result->emplace_back(status);
  }

  if (!transaction.Commit()) {
    return base::MakeUnique<std::vector<ItemActionStatus>>(
        urls.size(), ItemActionStatus::STORE_ERROR);
  }

  return result;
}
}  // namespace

CreatePrefetchItemsCommandSql::CreatePrefetchItemsCommandSql(
    SqlCommandExecutor* command_executor)
    : command_executor_(command_executor) {}

CreatePrefetchItemsCommandSql::~CreatePrefetchItemsCommandSql() {}

void CreatePrefetchItemsCommandSql::Execute(
    const std::string& name_space,
    const std::vector<PrefetchURL>& urls,
    const ResultCallback& result_callback) {
  command_executor_->Execute(
      base::BindOnce(&CreatePrefetchItemsSync, name_space, urls),
      result_callback);
}
}  // namespace offline_pages
