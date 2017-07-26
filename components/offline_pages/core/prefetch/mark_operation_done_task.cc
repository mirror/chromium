// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/mark_operation_done_task.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

bool UpdatePrefetchItemsSync(sql::Connection* db,
                             const std::string& operation_name) {
  static const char kSql[] =
      "UPDATE prefetch_items SET state = ?, freshness_time = ?"
      " WHERE state = ? AND operation_name = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::RECEIVED_GCM));
  statement.BindInt64(1, base::Time::Now().ToInternalValue());
  statement.BindInt(2, static_cast<int>(PrefetchItemState::AWAITING_GCM));
  statement.BindString(3, operation_name);

  return statement.Run();
}

// Will hold the actual SQL implementation for marking a MarkOperationDone
// attempt in the database.
MarkOperationDoneTask::Result MarkOperationCompletedOnServerSync(
    const std::string& operation_name,
    sql::Connection* db) {
  if (!db)
    return MarkOperationDoneTask::Result::STORE_ERROR;

  sql::Transaction transaction(db);
  if (transaction.Begin() && UpdatePrefetchItemsSync(db, operation_name) &&
      transaction.Commit()) {
    return MarkOperationDoneTask::Result::UPDATED;
  }
  return MarkOperationDoneTask::Result::STORE_ERROR;
}

}  // namespace

MarkOperationDoneTask::MarkOperationDoneTask(PrefetchStore* prefetch_store,
                                             const std::string& operation_name)
    : prefetch_store_(prefetch_store),
      operation_name_(operation_name),
      weak_factory_(this) {}

MarkOperationDoneTask::~MarkOperationDoneTask() {}

void MarkOperationDoneTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&MarkOperationCompletedOnServerSync, operation_name_),
      base::BindOnce(&MarkOperationDoneTask::Done, weak_factory_.GetWeakPtr()));
}

void MarkOperationDoneTask::Done(Result result) {
  result_ = result;
  // TODO(dewittj): UMA?
  TaskComplete();
}

}  // namespace offline_pages
