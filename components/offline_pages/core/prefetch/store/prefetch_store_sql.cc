// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_sql.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

// This is a macro instead of a const so that
// it can be used inline in other SQL statements below.
#define PREFETCH_ITEMS_TABLE_NAME "prefetch_items"

bool CreatePrefetchItemsTable(sql::Connection* db) {
  const char kSql[] = "CREATE TABLE IF NOT EXISTS " PREFETCH_ITEMS_TABLE_NAME
                      "(offline_id INTEGER PRIMARY KEY NOT NULL,"
                      " state INTEGER NOT NULL DEFAULT 0,"
                      " request_archive_attempt_count INTEGER NOT NULL,"
                      " archive_body_length INTEGER_NOT_NULL,"
                      " creation_time INTEGER NOT NULL,"
                      " freshness_time INTEGER NOT NULL,"
                      " error_code INTEGER NOT NULL,"
                      " guid VARCHAR NOT NULL DEFAULT '',"
                      " client_namespace VARCHAR NOT NULL DEFAULT '',"
                      " client_id VARCHAR NOT NULL DEFAULT '',"
                      " requested_url VARCHAR NOT NULL DEFAULT '',"
                      " final_archived_url VARCHAR NOT NULL DEFAULT '',"
                      " operation_name VARCHAR NOT NULL DEFAULT '',"
                      " archive_body_name VARCHAR NOT NULL DEFAULT ''"
                      ")";
  return db->Execute(kSql);
}

bool CreateSchema(sql::Connection* db) {
  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  if (!db->DoesTableExist(PREFETCH_ITEMS_TABLE_NAME)) {
    if (!CreatePrefetchItemsTable(db))
      return false;
  }

  // This would be a great place to add indices when we need them.
  return transaction.Commit();
}

/*
ItemActionStatus Insert(sql::Connection* db, const PrefetchItem& item) {
  // Using 'INSERT OR FAIL' or 'INSERT OR ABORT' in the query below causes debug
  // builds to DLOG.
  const char kSql[] =
      "INSERT OR IGNORE INTO " PREFETCH_ITEMS_TABLE_NAME
      " (offline_id, guid, client_namespace, client_id, state, requested_url,"
      " final_archived_url, request_archive_attempt_count, operation_name,"
      " archive_body_name, archive_body_length, creation_time, freshness_time,"
      " error_code)"
      " VALUES "
      " (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, item.offline_id);
  statement.BindString(1, item.guid);
  statement.BindString(2, item.client_id.name_space);
  statement.BindString(3, item.client_id.id);
  statement.BindInt(4, static_cast<int>(item.state));
  statement.BindString(5, item.url.spec());
  statement.BindString(6, item.final_archived_url.spec());
  statement.BindInt(7, static_cast<int>(item.request_archive_attempt_count));
  statement.BindString(8, item.operation_name);
  statement.BindString(9, item.archive_body_name);
  statement.BindInt64(10, item.archive_body_length);
  statement.BindInt64(11, item.creation_time.ToInternalValue());
  statement.BindInt64(12, item.freshness_time.ToInternalValue());
  statement.BindInt(13, static_cast<int>(item.error_code));

  if (!statement.Run())
    return ItemActionStatus::STORE_ERROR;
  if (db->GetLastChangeCount() == 0)
    return ItemActionStatus::ALREADY_EXISTS;
  return ItemActionStatus::SUCCESS;
}

bool Update(sql::Connection* db, const PrefetchItem& item) {
  const char kSql[] =
      "UPDATE OR IGNORE " PREFETCH_ITEMS_TABLE_NAME
      " SET guid = ?, client_namespace = ?, client_id = ?, state = ?,"
      " requested_url = ?, final_archived_url = ?,"
      " request_archive_attempt_counti = ?, operation_name = ?,"
      " archive_body_name = ?, archive_body_length = ?, creation_time = ?,"
      " freshness_time = ?, error_code = ?"
      " WHERE offline_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, item.guid);
  statement.BindString(1, item.client_id.name_space);
  statement.BindString(2, item.client_id.id);
  statement.BindInt(3, static_cast<int>(item.state));
  statement.BindString(4, item.url.spec());
  statement.BindString(5, item.final_archived_url.spec());
  statement.BindInt(6, static_cast<int>(item.request_archive_attempt_count));
  statement.BindString(7, item.operation_name);
  statement.BindString(8, item.archive_body_name);
  statement.BindInt64(9, item.archive_body_length);
  statement.BindInt64(10, item.creation_time.ToInternalValue());
  statement.BindInt64(11, item.freshness_time.ToInternalValue());
  statement.BindInt(12, static_cast<int>(item.error_code));
  statement.BindInt64(13, item.offline_id);

  return statement.Run() && db->GetLastChangeCount() > 0;
}
*/

bool InitDatabase(sql::Connection* db, base::FilePath path) {
  db->set_page_size(4096);
  db->set_cache_size(500);
  db->set_histogram_tag("PrefetchStore");
  db->set_exclusive_locking();

  base::File::Error err;
  if (!base::CreateDirectoryAndGetError(path.DirName(), &err)) {
    LOG(ERROR) << "Failed to create offline pages db directory: "
               << base::File::ErrorToString(err);
    return false;
  }
  if (!db->Open(path)) {
    LOG(ERROR) << "Failed to open database";
    return false;
  }
  db->Preload();

  return CreateSchema(db);
}

void OpenConnectionSync(sql::Connection* db,
                        scoped_refptr<base::SingleThreadTaskRunner> runner,
                        const base::FilePath& path,
                        const base::Callback<void(bool)>& callback) {
  bool success = InitDatabase(db, path);
  runner->PostTask(FROM_HERE, base::Bind(callback, success));
}

}  // anonymous namespace

PrefetchStoreSQL::PrefetchStoreSQL(
    scoped_refptr<base::SequencedTaskRunner> background_task_runner,
    const base::FilePath& path)
    : background_task_runner_(std::move(background_task_runner)),
      db_file_path_(path.AppendASCII("PrefetchStore.db")),
      state_(StoreState::NOT_LOADED),
      weak_ptr_factory_(this) {}

PrefetchStoreSQL::~PrefetchStoreSQL() {
  if (db_.get() &&
      !background_task_runner_->DeleteSoon(FROM_HERE, db_.release())) {
    DLOG(WARNING) << "SQL database will not be deleted.";
  }
}

void PrefetchStoreSQL::Initialize(const base::Callback<void(bool)>& callback) {
  DCHECK(!db_);
  db_.reset(new sql::Connection());
  background_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OpenConnectionSync, db_.get(),
                 base::ThreadTaskRunnerHandle::Get(), db_file_path_,
                 base::Bind(&PrefetchStoreSQL::OnOpenConnectionDone,
                            weak_ptr_factory_.GetWeakPtr(), callback)));
}

StoreState PrefetchStoreSQL::state() const {
  return state_;
}

void PrefetchStoreSQL::OnOpenConnectionDone(
    const base::Callback<void(bool)>& callback,
    bool success) {
  DCHECK(db_.get());
  state_ = success ? StoreState::LOADED : StoreState::FAILED_LOADING;
  callback.Run(success);
}

bool PrefetchStoreSQL::CheckDb() const {
  return db_ && state_ == StoreState::LOADED;
}

}  // namespace offline_pages
