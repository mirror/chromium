// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/sent_get_operation_cleanup_task.h"

#include <set>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

std::unique_ptr<std::vector<std::string>> GetRetribleOperationsSync(
    int max_attempts,
    sql::Connection* db) {
  static const char kSql[] =
      "SELECT operation_name"
      " FROM prefetch_items"
      " WHERE state = ? AND get_operation_attempts <= ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::SENT_GET_OPERATION));
  statement.BindInt(1, max_attempts);

  std::unique_ptr<std::vector<std::string>> operation_names;
  while (statement.Step()) {
    if (!operation_names)
      operation_names = base::MakeUnique<std::vector<std::string>>();
    operation_names->push_back(statement.ColumnString(0));
  }
  return operation_names;
}

bool UpdateToReceivedGCMStateSync(const std::string& operation_name,
                                  sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?"
      " WHERE operation_name = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::RECEIVED_GCM));
  statement.BindString(1, operation_name);

  return statement.Run();
}

bool RetryGetOperationSync(
    std::unique_ptr<std::set<std::string>> ongoing_operation_names,
    int max_attempts,
    sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  std::unique_ptr<std::vector<std::string>> tracked_operation_names =
      GetRetribleOperationsSync(max_attempts, db);
  if (!tracked_operation_names)
    return false;

  for (const auto& tracked_operation_name : *tracked_operation_names) {
    if (ongoing_operation_names &&
        ongoing_operation_names->count(tracked_operation_name))
      continue;
    if (!UpdateToReceivedGCMStateSync(tracked_operation_name, db))
      return false;
  }

  if (!transaction.Commit())
    return false;

  return true;
}

bool FailGetOperationSync(int max_attempts, sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?, error_code = ?"
      " WHERE state = ? AND get_operation_attempts > ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(
      1, static_cast<int>(
             PrefetchItemErrorCode::GET_OPERATION_MAX_ATTEMPTS_REACHED));
  statement.BindInt(2, static_cast<int>(PrefetchItemState::SENT_GET_OPERATION));
  statement.BindInt(3, max_attempts);

  return statement.Run();
}

bool CleanupGetOperationSync(
    std::unique_ptr<std::set<std::string>> ongoing_operation_names,
    int max_attempts,
    sql::Connection* db) {
  if (!db)
    return false;

  RetryGetOperationSync(std::move(ongoing_operation_names), max_attempts, db);
  FailGetOperationSync(max_attempts, db);
  return true;
}

}  // namespace

// static
const int SentGetOperationCleanupTask::kMaxGetOperationAttempts = 3;

SentGetOperationCleanupTask::SentGetOperationCleanupTask(
    PrefetchStore* prefetch_store,
    PrefetchNetworkRequestFactory* request_factory)
    : prefetch_store_(prefetch_store),
      request_factory_(request_factory),
      weak_ptr_factory_(this) {}

SentGetOperationCleanupTask::~SentGetOperationCleanupTask() {}

void SentGetOperationCleanupTask::Run() {
  std::unique_ptr<std::set<std::string>> ongoing_operation_names =
      request_factory_->GetAllOperationNamesRequested();

  prefetch_store_->Execute(
      base::BindOnce(&CleanupGetOperationSync,
                     std::move(ongoing_operation_names),
                     kMaxGetOperationAttempts),
      base::BindOnce(&SentGetOperationCleanupTask::OnFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SentGetOperationCleanupTask::OnFinished(bool success) {
  TaskComplete();
}

}  // namespace offline_pages
