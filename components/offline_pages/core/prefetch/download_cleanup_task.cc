// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/download_cleanup_task.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

std::unique_ptr<std::vector<std::string>> GetAllOutstandingDownloadsSync(
    sql::Connection* db) {
  static const char kSql[] =
      "SELECT guid"
      " FROM prefetch_items"
      " WHERE state = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADING));

  std::unique_ptr<std::vector<std::string>> download_ids;
  while (statement.Step()) {
    if (!download_ids)
      download_ids = base::MakeUnique<std::vector<std::string>>();
    download_ids->push_back(statement.ColumnString(0));
  }
  return download_ids;
}

bool UpdateDownloadSync(const std::string& download_id,
                        int max_attempts,
                        sql::Connection* db) {
  // For all items in DOWNLOADING state:
  // * transit to RECEIVED_BUNDLE state if not exceeding maximum attempts
  // * transit to FINISHED state with error_code set otherwise.
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = CASE WHEN download_initiation_attempts < ? THEN ?"
      "                  ELSE ? END,"
      "     error_code = CASE WHEN download_initiation_attempts < ? "
      "                       THEN error_code ELSE ? END"
      " WHERE guid = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, max_attempts);
  statement.BindInt(1, static_cast<int>(PrefetchItemState::RECEIVED_BUNDLE));
  statement.BindInt(2, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(3, max_attempts);
  statement.BindInt(
      4,
      static_cast<int>(PrefetchItemErrorCode::DOWNLOAD_MAX_ATTEMPTS_REACHED));
  statement.BindString(5, download_id);

  return statement.Run();
}

bool MarkDownloadAsCompletedSync(const PrefetchDownloadResult& download_result,
                                 sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?, file_path = ?, file_size = ?"
      " WHERE guid = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADED));
  statement.BindString(1, download_result.file_path.AsUTF8Unsafe());
  statement.BindInt64(2, download_result.file_size);
  statement.BindString(3, download_result.download_id);

  return statement.Run();
}

bool CleanupDownloadsSync(
    int max_attempts,
    const std::vector<std::string>& outstanding_download_ids,
    const std::vector<PrefetchDownloadResult>& success_downloads,
    sql::Connection* db) {
  if (!db)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  std::unique_ptr<std::vector<std::string>> tracked_download_ids =
      GetAllOutstandingDownloadsSync(db);
  if (!tracked_download_ids)
    return true;

  for (const auto& tracked_download_id : *tracked_download_ids) {
    // Mark ongoing downloads as completed if told by download system.
    const auto& iter = std::find_if(
        success_downloads.begin(), success_downloads.end(),
        [tracked_download_id](const PrefetchDownloadResult& download) {
          return download.download_id == tracked_download_id;
        });
    if (iter != success_downloads.end()) {
      if (!MarkDownloadAsCompletedSync(*iter, db))
        return false;
      continue;
    }

    // If the download is in progress, no change.
    if (std::find(outstanding_download_ids.begin(),
                  outstanding_download_ids.end(),
                  tracked_download_id) != outstanding_download_ids.end()) {
      continue;
    }

    // Otherwise, update the state to either retry the opeation or error out.
    if (!UpdateDownloadSync(tracked_download_id, max_attempts, db))
      return false;
  }

  return transaction.Commit();
}

}  // namespace

// static
const int DownloadCleanupTask::kMaxDownloadAttempts = 3;

DownloadCleanupTask::DownloadCleanupTask(
    PrefetchStore* prefetch_store,
    const std::vector<std::string>& outstanding_download_ids,
    const std::vector<PrefetchDownloadResult>& success_downloads)
    : prefetch_store_(prefetch_store),
      outstanding_download_ids_(outstanding_download_ids),
      success_downloads_(success_downloads),
      weak_ptr_factory_(this) {}

DownloadCleanupTask::~DownloadCleanupTask() {}

void DownloadCleanupTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&CleanupDownloadsSync, kMaxDownloadAttempts,
                     outstanding_download_ids_, success_downloads_),
      base::BindOnce(&DownloadCleanupTask::OnFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DownloadCleanupTask::OnFinished(bool success) {
  TaskComplete();
}

}  // namespace offline_pages
