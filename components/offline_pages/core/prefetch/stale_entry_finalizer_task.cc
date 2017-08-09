// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/stale_entry_finalizer_task.h"

#include <vector>

#include "base/bind.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

// All static.
const std::vector<PrefetchItemState>
    StaleEntryFinalizerTask::kBucket1StatesToExpire = {
        PrefetchItemState::NEW_REQUEST,
};
const std::vector<PrefetchItemState>
    StaleEntryFinalizerTask::kBucket2StatesToExpire = {
        PrefetchItemState::AWAITING_GCM, PrefetchItemState::RECEIVED_GCM,
        PrefetchItemState::RECEIVED_BUNDLE,
};
const std::vector<PrefetchItemState>
    StaleEntryFinalizerTask::kBucket3StatesToExpire = {
        PrefetchItemState::DOWNLOADING,
};

// All static.
const base::TimeDelta StaleEntryFinalizerTask::kBucket1FreshPeriod =
    base::TimeDelta::FromDays(1);
const base::TimeDelta StaleEntryFinalizerTask::kBucket2FreshPeriod =
    base::TimeDelta::FromDays(1);
const base::TimeDelta StaleEntryFinalizerTask::kBucket3FreshPeriod =
    base::TimeDelta::FromDays(2);

namespace {

auto& kBucket1FreshPeriod = StaleEntryFinalizerTask::kBucket1FreshPeriod;
auto& kBucket2FreshPeriod = StaleEntryFinalizerTask::kBucket2FreshPeriod;
auto& kBucket3FreshPeriod = StaleEntryFinalizerTask::kBucket3FreshPeriod;
auto& kBucket1StatesToExpire = StaleEntryFinalizerTask::kBucket1StatesToExpire;
auto& kBucket2StatesToExpire = StaleEntryFinalizerTask::kBucket2StatesToExpire;
auto& kBucket3StatesToExpire = StaleEntryFinalizerTask::kBucket3StatesToExpire;

bool FindStaleItems(const PrefetchItemState state,
                    const base::Time earliest_fresh_time,
                    std::vector<int64_t>* offline_ids,
                    sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id FROM prefetch_items"
      " WHERE state = ? AND freshness_time < ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(state));
  statement.BindInt64(1, ToDatabaseTime(earliest_fresh_time));

  while (statement.Step()) {
    offline_ids->push_back(statement.ColumnInt64(0));
  }
  return statement.Succeeded();
}

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

bool FindAndFinalizeStaleItems(
    const std::vector<PrefetchItemState>& bucket_states,
    const base::Time earliest_fresh_time,
    PrefetchItemErrorCode error_code,
    sql::Connection* db) {
  std::vector<int64_t> offline_ids;
  for (const PrefetchItemState state : bucket_states) {
    if (!FindStaleItems(state, earliest_fresh_time, &offline_ids, db))
      return false;
  }
  for (const int64_t& offline_id : offline_ids) {
    if (!FinalizeItem(offline_id, error_code, db))
      return false;
  }
  return true;
}

bool FinalizeStaleEntriesSync(StaleEntryFinalizerTask::NowCallback now_callback,
                              sql::Connection* db) {
  if (!db)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  base::Time now = now_callback.Run();
  if (FindAndFinalizeStaleItems(
          kBucket1StatesToExpire, now - kBucket1FreshPeriod,
          PrefetchItemErrorCode::BECAME_STALE_WHILE_AT_BUCKET_1, db) &&
      FindAndFinalizeStaleItems(
          kBucket2StatesToExpire, now - kBucket2FreshPeriod,
          PrefetchItemErrorCode::BECAME_STALE_WHILE_AT_BUCKET_2, db) &&
      FindAndFinalizeStaleItems(
          kBucket3StatesToExpire, now - kBucket3FreshPeriod,
          PrefetchItemErrorCode::BECAME_STALE_WHILE_AT_BUCKET_3, db)) {
    return transaction.Commit();
  }

  // If any of the FindAndFinalizeStaleItems steps failed the transaction will
  // not be committed and automatically reverted upon destruction.
  return false;
}

}  // namespace

StaleEntryFinalizerTask::StaleEntryFinalizerTask(PrefetchStore* prefetch_store)
    : prefetch_store_(prefetch_store),
      now_callback_(base::BindRepeating(&base::Time::Now)),
      weak_ptr_factory_(this) {}

StaleEntryFinalizerTask::~StaleEntryFinalizerTask() {}

void StaleEntryFinalizerTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&FinalizeStaleEntriesSync, now_callback_),
      base::BindOnce(&StaleEntryFinalizerTask::OnExpirationConcluded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void StaleEntryFinalizerTask::OnExpirationConcluded(bool success) {
  ran_successfully_ = success;
  TaskComplete();
}

}  // namespace offline_pages
