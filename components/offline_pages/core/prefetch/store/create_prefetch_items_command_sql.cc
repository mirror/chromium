// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/create_prefetch_items_command_sql.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

typedef base::Callback<void(const std::vector<ItemActionStatus>&)>
    CreateCallback;

void CreatePrefetchItemsSync(sql::Connection* db,
                             const std::string& name_space,
                             const std::vector<PrefetchURL>& urls,
                             scoped_refptr<base::SingleThreadTaskRunner> runner,
                             const CreateCallback& callback) {
  std::vector<ItemActionStatus> result;

  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    runner->PostTask(
        FROM_HERE,
        base::Bind(callback, std::vector<ItemActionStatus>(
                                 urls.size(), ItemActionStatus::STORE_ERROR)));
    return;
  }

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
    result.emplace_back(status);
  }

  if (!transaction.Commit()) {
    runner->PostTask(
        FROM_HERE,
        base::Bind(callback, std::vector<ItemActionStatus>(
                                 urls.size(), ItemActionStatus::STORE_ERROR)));
    return;
  }

  runner->PostTask(FROM_HERE, base::Bind(callback, std::move(result)));
}
}  // namespace

CreatePrefetchItemsCommandSql::CreatePrefetchItemsCommandSql()
    : weak_ptr_factory_(this) {}

CreatePrefetchItemsCommandSql::~CreatePrefetchItemsCommandSql() {}

void CreatePrefetchItemsCommandSql::SetUrlsToPrefetch(
    const std::string& name_space,
    const std::vector<PrefetchURL>& urls) {
  namespace_ = name_space;
  prefetch_urls_ = urls;
}

void CreatePrefetchItemsCommandSql::Execute(
    sql::Connection* db,
    scoped_refptr<base::SingleThreadTaskRunner> background_runner) {
  background_runner->PostTask(
      FROM_HERE,
      base::Bind(
          &CreatePrefetchItemsSync, db, namespace_, prefetch_urls_,
          base::ThreadTaskRunnerHandle::Get(),
          base::Bind(&CreatePrefetchItemsCommandSql::CreatePrefetchItemsDone,
                     weak_ptr_factory_.GetWeakPtr())));
}

void CreatePrefetchItemsCommandSql::CreatePrefetchItemsDone(
    const std::vector<ItemActionStatus>& result) {}

}  // namespace offline_pages
