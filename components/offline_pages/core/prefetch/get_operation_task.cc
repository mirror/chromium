// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/get_operation_task.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

GetOperationTask::OperationResultList MakeError() {
  return std::make_pair(GetOperationTask::StoreResult::STORE_ERROR,
                        std::vector<std::string>());
}

bool UpdatePrefetchItemsForOperationSync(sql::Connection* db,
                                         const std::string& operation_name) {
  static const char kSql[] =
      "UPDATE prefetch_items SET"
      " state = ?"
      // TODO(dewittj): Rebase onto carlosk's patch.
      //", get_operation_attempt_count = get_operation_attempt_count + 1"
      " WHERE state = ? AND operation_name = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::SENT_GET_OPERATION));
  statement.BindInt(1, static_cast<int>(PrefetchItemState::RECEIVED_GCM));
  statement.BindString(2, operation_name);

  return statement.Run();
}

std::vector<std::string> AvailableOperationsSync(sql::Connection* db) {
  static const char kSql[] =
      "SELECT DISTINCT operation_name FROM prefetch_items WHERE state = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::RECEIVED_GCM));

  std::vector<std::string> operations;
  while (statement.Step()) {
    operations.push_back(statement.ColumnString(0));  // operation_name
  }
  return operations;
}

GetOperationTask::OperationResultList SelectOperationsToFetch(
    sql::Connection* db) {
  if (!db)
    return MakeError();

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return MakeError();

  auto operations = AvailableOperationsSync(db);

  for (std::string operation : operations) {
    if (!UpdatePrefetchItemsForOperationSync(db, operation))
      return MakeError();  // Rollback.
  }

  if (!transaction.Commit())
    return MakeError();

  return std::make_pair(GetOperationTask::StoreResult::SUCCESS, operations);
}

}  // namespace

GetOperationTask::GetOperationTask(
    PrefetchStore* store,
    PrefetchNetworkRequestFactory* request_factory,
    const PrefetchRequestFinishedCallback& callback)
    : prefetch_store_(store),
      request_factory_(request_factory),
      callback_(callback),
      weak_factory_(this) {}

GetOperationTask::~GetOperationTask() {}

void GetOperationTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&SelectOperationsToFetch),
      base::BindOnce(&GetOperationTask::StartGetOperationRequests,
                     weak_factory_.GetWeakPtr()));
}

void GetOperationTask::StartGetOperationRequests(OperationResultList list) {
  if (std::get<0>(list) == StoreResult::STORE_ERROR) {
    TaskComplete();
    return;
  }

  std::vector<std::string>& operation_names = std::get<1>(list);
  for (std::string& operation : operation_names) {
    request_factory_->MakeGetOperationRequest(operation, callback_);
  }

  TaskComplete();
}

}  // namespace offline_pages
