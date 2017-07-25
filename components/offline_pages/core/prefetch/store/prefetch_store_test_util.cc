// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "url/gurl.h"

namespace offline_pages {
namespace {

int CountPrefetchItemsSync(sql::Connection* db) {
  // Not starting transaction as this is a single read.
  static const char kSql[] = "SELECT COUNT(offline_id) FROM prefetch_items";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  if (statement.Step())
    return statement.ColumnInt(0);

  return kStoreCommandFailed;
}

std::size_t GetAllItemsSync(std::set<PrefetchItem>* items,
                            sql::Connection* db) {
  // Not starting transaction as this is a single read.
  static const char kSql[] = "SELECT * FROM prefetch_items";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  while (statement.Step()) {
    PrefetchItem loaded_item;
    LoadPrefetchItem(statement, &loaded_item);
    items->insert(loaded_item);
  }
  return items->size();
}

int UpdateItemsStateSync(const std::string& name_space,
                         const std::string& url,
                         PrefetchItemState state,
                         sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?"
      " WHERE client_namespace = ? AND requested_url = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(state));
  statement.BindString(1, name_space);
  statement.BindString(2, url);
  if (statement.Run())
    return db->GetLastChangeCount();

  return kStoreCommandFailed;
}

bool InsertItemSync(const PrefetchItem& item, sql::Connection* db) {
  // Not starting transaction as this is a single write.
  static const char kSql[] =
      "INSERT INTO prefetch_items"
      " (offline_id, state, generate_bundle_attempts, get_operation_attempts,"
      "  download_operation_attempts, archive_body_length,"
      "  creation_time, freshness_time, error_code, guid, client_namespace,"
      "  client_id, requested_url, final_archived_url, operation_name,"
      "  archive_body_name)"
      " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, item.offline_id);
  statement.BindInt(1, static_cast<int>(item.state));
  statement.BindInt(2, item.generate_bundle_attempts);
  statement.BindInt(3, item.get_operation_attempts);
  statement.BindInt(4, item.download_operation_attempts);
  statement.BindInt64(5, item.archive_body_length);
  statement.BindInt64(6, item.creation_time.since_origin().InMicroseconds());
  statement.BindInt64(7, item.freshness_time.since_origin().InMicroseconds());
  statement.BindInt(8, static_cast<int>(item.error_code));
  statement.BindString(9, item.guid);
  statement.BindString(10, item.client_id.name_space);
  statement.BindString(11, item.client_id.id);
  statement.BindString(12, item.url.spec());
  statement.BindString(13, item.final_archived_url.spec());
  statement.BindString(14, item.operation_name);
  statement.BindString(15, item.archive_body_name);

  return statement.Run();
}

}  // namespace

PrefetchStoreTestUtil::PrefetchStoreTestUtil() {}

PrefetchStoreTestUtil::~PrefetchStoreTestUtil() = default;

void PrefetchStoreTestUtil::BuildStore() {
  if (!temp_directory_.CreateUniqueTempDir())
    DVLOG(1) << "temp_directory_ not created";

  store_.reset(new PrefetchStore(base::ThreadTaskRunnerHandle::Get(),
                                 temp_directory_.GetPath()));
}

void PrefetchStoreTestUtil::BuildStoreInMemory() {
  store_.reset(new PrefetchStore(base::ThreadTaskRunnerHandle::Get()));
}

std::unique_ptr<PrefetchStore> PrefetchStoreTestUtil::ReleaseStore() {
  return std::move(store_);
}

void PrefetchStoreTestUtil::DeleteStore() {
  store_.reset();
  if (temp_directory_.IsValid()) {
    if (!temp_directory_.Delete())
      DVLOG(1) << "temp_directory_ not created";
  }
}

void PrefetchStoreTestUtil::CountPrefetchItems(
    PrefetchStore::ResultCallback<int> result_callback) {
  store_->Execute(base::BindOnce(&CountPrefetchItemsSync),
                  std::move(result_callback));
}

void PrefetchStoreTestUtil::GetAllItems(
    std::set<PrefetchItem>* all_items,
    PrefetchStore::ResultCallback<std::size_t> result_callback) {
  store_->Execute(base::BindOnce(&GetAllItemsSync, all_items),
                  std::move(result_callback));
}

void PrefetchStoreTestUtil::ZombifyPrefetchItem(
    const std::string& name_space,
    const GURL& url,
    PrefetchStore::ResultCallback<int> result_callback) {
  store_->Execute(base::BindOnce(&UpdateItemsStateSync, name_space, url.spec(),
                                 PrefetchItemState::ZOMBIE),
                  std::move(result_callback));
}

void PrefetchStoreTestUtil::InsertItem(
    const PrefetchItem& item,
    PrefetchStore::ResultCallback<bool> result_callback) {
  store_->Execute(base::BindOnce(&InsertItemSync, item),
                  std::move(result_callback));
}

}  // namespace offline_pages
