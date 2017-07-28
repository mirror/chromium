// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/entry_expiration_task.h"

#include <map>
#include <memory>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
// #include "base/strings/string_util.h"
#include "base/time/time.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

void FindStalePrefetchItems(const std::string& bucket_states, const base::Time earliest_fresh_time, std::vector<PrefetchItem>* items, sql::Connection* db) {
  static const char kSql[] =
      "SELECT * FROM prefetch_items WHERE state in (?) AND freshness_time < ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, bucket_states);
  statement.BindInt64(1, earliest_fresh_time.ToInternalValue());

  while (statement.Step()) {
    items->push_back(PrefetchItem());
    LoadPrefetchItem(statement, &(items->at(items->size()-1)));
  }
}

bool FinalizeItemSync(int64_t offline_id,
                         PrefetchItemErrorCode error_code,
                         sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items SET (state = ?, error_code = ?) WHERE offline_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(1, static_cast<int>(error_code));
  statement.BindInt64(2, offline_id);
  return statement.Run();
}

// TODO
std::vector<PrefetchItem> ExpireEntriesSync(EntryExpirationTask::NowCallback now_callback, sql::Connection* db) {
  std::vector<PrefetchItem> expired_items;
  if (!db)
    return expired_items;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return expired_items;

  base::Time now = now_callback.Run();
  FindStalePrefetchItems(kBucket1StatesString, now - kBucket1FreshPeriod, &expired_items, db);
  FindStalePrefetchItems(kBucket2StatesString, now - kBucket2FreshPeriod, &expired_items, db);
  FindStalePrefetchItems(kBucket3StatesString, now - kBucket3FreshPeriod, &expired_items, db);
  for (auto items_it = expired_items.begin(); items_it != expired_items.end(); ++items_it) {
    if (!FinalizeItemSync((*items_it).offline_id, PrefetchItemErrorCode::EXPIRED, db)) {
      expired_items.erase(items_it);
    }
  }
  std::size_t finished_items_count = expired_items.size();

  return expired_items;
}

std::string JoinStateList(std::vector<PrefetchItemState> states) {
  if (!states.size())
    return std::string();

  std::string joined(base::IntToString(static_cast<int>(states[0])));
  for (std::size_t i = 1; i < states.size(); ++i)
    joined.append(", ").append(base::IntToString(static_cast<int>(states[i])));
  return joined;
}

}  // namespace

const std::string kBucket1StatesString = JoinStateList(kBucket1States);
const std::string kBucket2StatesString = JoinStateList(kBucket2States);
const std::string kBucket3StatesString = JoinStateList(kBucket3States);

const base::TimeDelta kBucket1FreshPeriod = base::TimeDelta::FromDays(1);
const base::TimeDelta kBucket2FreshPeriod = base::TimeDelta::FromDays(1);
const base::TimeDelta kBucket3FreshPeriod = base::TimeDelta::FromDays(2);

EntryExpirationTask::EntryExpirationTask(PrefetchStore* prefetch_store)
    : prefetch_store_(prefetch_store),
      now_callback_(base::BindRepeating(&base::Time::Now)),
      weak_ptr_factory_(this) {}

EntryExpirationTask::~EntryExpirationTask() {}

void EntryExpirationTask::Run() {
  prefetch_store_->Execute(base::BindOnce(&ExpireEntriesSync, now_callback_),
                           base::BindOnce(&EntryExpirationTask::OnExpirationConcluded,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void EntryExpirationTask::OnExpirationConcluded(std::vector<PrefetchItem> expired_items) {
  // TODO
  TaskComplete();
}

}  // namespace offline_pages
