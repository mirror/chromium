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
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {
// Name of the table with prefetch items.
const char kPrefetchItemsTableName[] = "prefetch_items";
const char kPrefetchStoreFileName[] = "PrefetchStore.db";

using InitializeCallback =
    base::Callback<void(InitializationStatus,
                        std::unique_ptr<sql::Connection>)>;

bool CreatePrefetchItemsTable(sql::Connection* db) {
  const char kSql[] =
      "CREATE TABLE IF NOT EXISTS prefetch_items"
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

  if (!db->DoesTableExist(kPrefetchItemsTableName)) {
    if (!CreatePrefetchItemsTable(db))
      return false;
  }

  // This would be a great place to add indices when we need them.
  return transaction.Commit();
}

bool PrepareDirectory(const base::FilePath& path) {
  base::File::Error error = base::File::FILE_OK;
  if (!base::DirectoryExists(path.DirName())) {
    if (!base::CreateDirectoryAndGetError(path.DirName(), &error)) {
      LOG(ERROR) << "Failed to create prefetch db directory: "
                 << base::File::ErrorToString(error);
      return false;
    }
  }
  return true;
}

// TODO(fgorski): This function and this part of the system in general could
// benefit from a better status code reportable through UMA to better capture
// the reason for failure, aiding the process of repeated attempts to
// open/initialize the database.
bool InitDatabase(sql::Connection* db,
                  const base::FilePath& path,
                  bool in_memory) {
  db->set_page_size(4096);
  db->set_cache_size(500);
  db->set_histogram_tag("PrefetchStore");
  db->set_exclusive_locking();

  if (!in_memory && !PrepareDirectory(path))
    return false;

  bool open_db_result = false;
  if (in_memory)
    open_db_result = db->OpenInMemory();
  else
    open_db_result = db->Open(path);

  if (open_db_result) {
    LOG(ERROR) << "Failed to open database";
    return false;
  }
  db->Preload();

  return CreateSchema(db);
}

// Entry point to the background initialization logic.
// TODO(fgorski): (Delayed) retries should happen here.
std::unique_ptr<sql::Connection> InitializeSync(const base::FilePath& path,
                                                bool in_memory) {
  std::unique_ptr<sql::Connection> db(new sql::Connection);
  if (!InitDatabase(db.get(), path, in_memory))
    db.reset();
  return db;
}

}  // anonymous namespace

// Explicit template instantiations (unit tests compilation depends on that).
template void PrefetchStoreSQL::Execute<int>(RunCallback<int>,
                                             ResultCallback<int>);

PrefetchStoreSQL::PrefetchStoreSQL(
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner)
    : blocking_task_runner_(std::move(blocking_task_runner)),
      in_memory_(true),
      initialization_status_(InitializationStatus::NOT_INITIALIZED),
      weak_ptr_factory_(this) {}

PrefetchStoreSQL::PrefetchStoreSQL(
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner,
    const base::FilePath& path)
    : blocking_task_runner_(std::move(blocking_task_runner)),
      db_file_path_(path.AppendASCII(kPrefetchStoreFileName)),
      in_memory_(false),
      initialization_status_(InitializationStatus::NOT_INITIALIZED),
      weak_ptr_factory_(this) {}

PrefetchStoreSQL::~PrefetchStoreSQL() {
  if (db_.get() &&
      !blocking_task_runner_->DeleteSoon(FROM_HERE, db_.release())) {
    DLOG(WARNING) << "SQL database will not be deleted.";
  }
}

template <typename T>
void PrefetchStoreSQL::Execute(RunCallback<T> run_callback,
                               ResultCallback<T> result_callback) {
  if (initialization_status_ == InitializationStatus::SUCCESS) {
    base::PostTaskAndReplyWithResult(
        blocking_task_runner_.get(), FROM_HERE,
        base::BindOnce(std::move(run_callback), db_.get()),
        std::move(result_callback));
    return;
  }

  pending_commands_.emplace_back(base::BindOnce(
      &PrefetchStoreSQL::Execute<T>, weak_ptr_factory_.GetWeakPtr(),
      std::move(run_callback), std::move(result_callback)));
  if (initialization_status_ == InitializationStatus::NOT_INITIALIZED)
    Initialize();
}

void PrefetchStoreSQL::Initialize() {
  DCHECK_EQ(initialization_status_, InitializationStatus::NOT_INITIALIZED);
  DCHECK(!db_);

  initialization_status_ = InitializationStatus::INITIALIZING;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&InitializeSync, db_file_path_, in_memory_),
      base::BindOnce(&PrefetchStoreSQL::OnInitializeDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PrefetchStoreSQL::OnInitializeDone(std::unique_ptr<sql::Connection> db) {
  initialization_status_ =
      db ? InitializationStatus::SUCCESS : InitializationStatus::FAILURE;
  db_ = std::move(db);

  if (db_) {
    // This should be enough at least for now, because access to the prefetch
    // store is guarded by a task queue, and we don't expect more than one
    // command to be in pending_commands_.
    while (pending_commands_.size() > 0) {
      std::move(pending_commands_.front()).Run();
      pending_commands_.erase(pending_commands_.begin());
    }
  }
}
}  // namespace offline_pages
