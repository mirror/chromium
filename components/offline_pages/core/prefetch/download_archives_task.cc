// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/download_archives_task.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/prefetch_downloader.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

using ItemsReadyForDownload = std::vector<std::pair<int64_t, std::string>>;

DownloadArchivesTask::DownloadItem MakeDownloadItem(
    const std::string& guid,
    const std::string& archive_body_name) {
  DownloadArchivesTask::DownloadItem item = {guid, archive_body_name};
  return item;
}

ItemsReadyForDownload FindItemsReadyForDownload(sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id, archive_body_name FROM prefetch_items"
      " WHERE state = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::RECEIVED_BUNDLE));

  ItemsReadyForDownload items_to_download;
  while (statement.Step()) {
    items_to_download.push_back(
        std::make_pair(statement.ColumnInt64(0), statement.ColumnString(1)));
  }

  return items_to_download;
}

bool MarkItemAsDownloading(sql::Connection* db,
                           int64_t offline_id,
                           const std::string& guid) {
  static const char kSql[] =
      "UPDATE prefetch_items SET state = ?, guid = ?,"
      " download_initiation_attempts = download_initiation_attempts + 1"
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADING));
  statement.BindString(1, guid);
  statement.BindInt64(2, offline_id);
  return statement.Run();
}

std::unique_ptr<DownloadArchivesTask::ItemsToDownload>
SelectAndMarkItemsForDownloadSync(sql::Connection* db) {
  if (!db)
    return nullptr;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return nullptr;

  // TODO(fgorski): Implement daily quota algorithm and filter out the items
  // that we want to start downloading below.

  ItemsReadyForDownload ready_items = FindItemsReadyForDownload(db);
  auto items_to_download =
      base::MakeUnique<DownloadArchivesTask::ItemsToDownload>();
  for (const auto& item : ready_items) {
    std::string guid = base::GenerateGUID();
    if (!MarkItemAsDownloading(db, item.first, guid))
      return nullptr;
    items_to_download->push_back(MakeDownloadItem(guid, item.second));
  }

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
