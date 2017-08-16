// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/sent_get_operation_cleanup_task.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {
namespace {

bool RetryGetOperationSync(int max_attempts, sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?"
      " WHERE state = ? AND get_operation_attempts <= ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::RECEIVED_GCM));
  statement.BindInt(1, static_cast<int>(PrefetchItemState::SENT_GET_OPERATION));
  statement.BindInt(2, max_attempts);

  return statement.Run();
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

}  // namespace

// static
const int SentGetOperationCleanupTask::kMaxGetOperationAttempts = 3;

SentGetOperationCleanupTask::SentGetOperationCleanupTask(
    PrefetchStore* prefetch_store)
    : prefetch_store_(prefetch_store), weak_ptr_factory_(this) {}

SentGetOperationCleanupTask::~SentGetOperationCleanupTask() {}

void SentGetOperationCleanupTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&RetryGetOperationSync, kMaxGetOperationAttempts),
      base::BindOnce(&SentGetOperationCleanupTask::OnRetryGetOperationSyncDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SentGetOperationCleanupTask::OnRetryGetOperationSyncDone(bool success) {
  prefetch_store_->Execute(
      base::BindOnce(&FailGetOperationSync, kMaxGetOperationAttempts),
      base::BindOnce(&SentGetOperationCleanupTask::OnFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SentGetOperationCleanupTask::OnFinished(bool success) {
  TaskComplete();
}

}  // namespace offline_pages
