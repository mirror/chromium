// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/download_archives_task.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "components/offline_pages/core/prefetch/prefetch_downloader.h"
#include "components/offline_pages/core/prefetch/store/prefetch_downloader_quota.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

const int kMaxConcurrentDownloads = 2;

struct ItemReadyForDownload {
  int64_t offline_id;
  std::string archive_body_name;
  int64_t archive_body_length;
};

using ItemsReadyForDownload = std::vector<ItemReadyForDownload>;

ItemsReadyForDownload FindItemsReadyForDownload(sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id, archive_body_name, archive_body_length"
      " FROM prefetch_items"
      " WHERE state = ?"
      " ORDER BY creation_time DESC";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::RECEIVED_BUNDLE));

  ItemsReadyForDownload items_to_download;
  while (statement.Step()) {
    items_to_download.push_back({statement.ColumnInt64(0),
                                 statement.ColumnString(1),
                                 statement.ColumnInt64(2)});
  }

  return items_to_download;
}

int CountDownloadsInProgress(sql::Connection* db) {
  static const char kSql[] =
      "SELECT COUNT(offline_id) FROM prefetch_items WHERE state = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADING));
  if (!statement.Step())
    return 0;
  return statement.ColumnInt(0);
}

bool MarkItemAsDownloading(sql::Connection* db,
                           int64_t offline_id,
                           const std::string& guid) {
  // Code below only changes freshness time once, when the archive download is
  // attempted for the first time. We don't want to perpetuate the lifetime of
  // the item longer than that, if we keep on retrying it.
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?,"
      "     guid = ?,"
      "     freshness_time = CASE WHEN download_initiation_attempts = 0 THEN ?"
      "                           ELSE freshness_time END,"
      "     download_initiation_attempts = download_initiation_attempts + 1"
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADING));
  statement.BindString(1, guid);
  statement.BindInt64(2, ToDatabaseTime(base::Time::Now()));
  statement.BindInt64(3, offline_id);
  return statement.Run();
}

std::unique_ptr<DownloadArchivesTask::ItemsToDownload>
SelectAndMarkItemsForDownloadSync(sql::Connection* db) {
  if (!db)
    return nullptr;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return nullptr;

  int64_t available_quota = PrefetchDownloaderQuota::GetAvailableQuota(db);
  int concurrent_downloads = CountDownloadsInProgress(db);

  ItemsReadyForDownload ready_items = FindItemsReadyForDownload(db);
  if (ready_items.empty())
    return nullptr;

  auto items_to_download =
      base::MakeUnique<DownloadArchivesTask::ItemsToDownload>();
  for (const auto& ready_item : ready_items) {
    // Concurrent downloads check.
    if (concurrent_downloads >= kMaxConcurrentDownloads)
      break;

    // Quota check.
    if (ready_item.archive_body_length > available_quota)
      continue;

    // Explicitly not reusing the GUID from the last archive download attempt
    // here.
    std::string guid = base::GenerateGUID();
    if (!MarkItemAsDownloading(db, ready_item.offline_id, guid))
      return nullptr;
    items_to_download->push_back({guid, ready_item.archive_body_name});
    ++concurrent_downloads;
  }

  // Write new remaining quota with date here.
  if (!PrefetchDownloaderQuota::SetAvailableQuota(db, available_quota))
    return nullptr;

  if (!transaction.Commit())
    return nullptr;

  return items_to_download;
}

}  // namespace

DownloadArchivesTask::DownloadArchivesTask(
    PrefetchStore* prefetch_store,
    PrefetchDownloader* prefetch_downloader)
    : prefetch_store_(prefetch_store),
      prefetch_downloader_(prefetch_downloader),
      weak_ptr_factory_(this) {
  DCHECK(prefetch_store_);
  DCHECK(prefetch_downloader_);
}

DownloadArchivesTask::~DownloadArchivesTask() = default;

void DownloadArchivesTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(SelectAndMarkItemsForDownloadSync),
      base::BindOnce(&DownloadArchivesTask::SendItemsToPrefetchDownloader,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DownloadArchivesTask::SendItemsToPrefetchDownloader(
    std::unique_ptr<ItemsToDownload> items_to_download) {
  if (items_to_download) {
    for (const auto& download_item : *items_to_download) {
      prefetch_downloader_->StartDownload(download_item.guid,
                                          download_item.archive_body_name);
    }
  }

  TaskComplete();
}

}  // namespace offline_pages
