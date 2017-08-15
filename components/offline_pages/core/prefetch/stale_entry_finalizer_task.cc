// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/stale_entry_finalizer_task.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/stale_entry_finalizer_delegate_impl.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

bool FinalizeItem(int64_t offline_id,
                  PrefetchItemErrorCode error_code,
                  sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items SET state = ?, error_code = ?"
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(1, static_cast<int>(error_code));
  statement.BindInt64(2, offline_id);
  return statement.Run();
}

bool FindAndFinalizeStaleItems(StaleEntryFinalizerTask::Bucket bucket,
                               base::Time now,
                               StaleEntryFinalizerTask::Delegate* delegate,
                               sql::Connection* db) {
  const std::vector<PrefetchItemState>& bucket_states =
      delegate->StatesToExpireForBucket(bucket);
  const int64_t earliest_fresh_db_time =
      ToDatabaseTime(now - delegate->FreshnessPeriodForBucket(bucket));
  for (const PrefetchItemState state : bucket_states) {
    static const char kSql[] =
        "SELECT offline_id FROM prefetch_items"
        " WHERE state = ? AND freshness_time < ?";
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindInt(0, static_cast<int>(state));
    statement.BindInt64(1, earliest_fresh_db_time);

    PrefetchItemErrorCode error_code = delegate->ErrorCodeForStaleEntry(state);
    while (statement.Step()) {
      if (!FinalizeItem(statement.ColumnInt64(0), error_code, db))
        return false;
    }
    if (!statement.Succeeded())
      return false;
  }
  return true;
}

bool FinalizeStaleEntriesSync(StaleEntryFinalizerTask::Delegate* delegate,
                              sql::Connection* db) {
  if (!db)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  base::Time now = delegate->Now();
  if (FindAndFinalizeStaleItems(StaleEntryFinalizerTask::BUCKET_1, now,
                                delegate, db) &&
      FindAndFinalizeStaleItems(StaleEntryFinalizerTask::BUCKET_2, now,
                                delegate, db) &&
      FindAndFinalizeStaleItems(StaleEntryFinalizerTask::BUCKET_3, now,
                                delegate, db)) {
    return transaction.Commit();
  }

  // If any of the FindAndFinalizeStaleItems steps failed the transaction will
  // not be committed and automatically reverted upon destruction.
  return false;
}

}  // namespace

StaleEntryFinalizerTask::StaleEntryFinalizerTask(PrefetchStore* prefetch_store)
    : prefetch_store_(prefetch_store),
      delegate_(base::MakeUnique<StaleEntryFinalizerDelegateImpl>()),
      weak_ptr_factory_(this) {
  DCHECK(prefetch_store_);
}

StaleEntryFinalizerTask::~StaleEntryFinalizerTask() {}

void StaleEntryFinalizerTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&FinalizeStaleEntriesSync, delegate_.get()),
      base::BindOnce(&StaleEntryFinalizerTask::OnExpirationConcluded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void StaleEntryFinalizerTask::SetDelegateForTesting(
    std::unique_ptr<StaleEntryFinalizerTask::Delegate> delegate) {
  delegate_ = std::move(delegate);
}

void StaleEntryFinalizerTask::OnExpirationConcluded(bool success) {
  ran_successfully_ = success;
  TaskComplete();
}

}  // namespace offline_pages
