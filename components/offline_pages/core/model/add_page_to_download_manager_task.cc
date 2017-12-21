// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/add_page_to_download_manager_task.h"

#include <stdint.h>

#include "base/bind.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/system_download_manager.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {
#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

bool SetDownloadIdSync(int64_t offline_id,
                       int64_t download_id,
                       sql::Connection* db) {
  DVLOG(0) << "@@@@@@ setting download ID " << __func__;
  if (!db)
    return false;

  const char kSql[] =
      "UPDATE OR IGNORE " OFFLINE_PAGES_TABLE_NAME
      " SET system_download_id = ? "  // TODO: Verify SQL INTEGER holds int64_t
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, download_id);
  statement.BindInt64(1, offline_id);
  return statement.Run();
}
}  // namespace

AddPageToDownloadManagerTask::AddPageToDownloadManagerTask(
    OfflinePageMetadataStoreSQL* store,
    SystemDownloadManager* download_manager,
    int64_t offline_id,
    const std::string& title,
    const std::string& description,
    const std::string& path,
    long length,
    const std::string& uri,
    const std::string& referer)
    : store_(store),
      title_(title),
      description_(description),
      path_(path),
      uri_(uri),
      referer_(referer),
      offline_id_(offline_id),
      length_(length),
      download_manager_(download_manager),
      weak_ptr_factory_(this) {}

AddPageToDownloadManagerTask::~AddPageToDownloadManagerTask() {}

void AddPageToDownloadManagerTask::Run() {
  // Check to see if we have a system download manager.
  if (!download_manager_->IsDownloadManagerInstalled())
    return;

  // Tell the download manager about our file, get back an id.
  long download_id = download_manager_->addCompletedDownload(
      title_, description_, path_, length_, uri_, referer_);

  // Add the download ID to the OfflinePageModel database.
  store_->Execute(base::BindOnce(&SetDownloadIdSync, offline_id_, download_id),
                  base::BindOnce(&AddPageToDownloadManagerTask::OnAddIdDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void AddPageToDownloadManagerTask::OnAddIdDone(bool result) {
  // There is no callback to the code that added the task, so we are done now.
  // Errors in the SQL statement is ignored, there isn't much that the calling
  // code can do to fix the situation if we can't write to the DB.
  TaskComplete();
}

}  // namespace offline_pages
