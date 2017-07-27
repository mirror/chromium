// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_archives_task.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "components/offline_pages/core/prefetch/prefetch_importer.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

std::vector<PrefetchItem> GetPrefetchItemsSync(sql::Connection* db) {
  static const char kSql[] = "SELECT * FROM prefetch_items WHERE state = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADED));

  std::vector<PrefetchItem> result;
  while (statement.Step()) {
    PrefetchItem item;
    MakePrefetchItem(statement, &item);
    result.push_back(item);
  }

  return result;
}

bool UpdatePrefetchItemSync(int64_t offline_id,
                            bool success,
                            sql::Connection* db) {
  static const char kSql[] =
      "UPDATE OR IGNORE prefetch_items"
      " SET state = ?, error_code = ?"
      " WHERE offline_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(
      1, static_cast<int>(success ? PrefetchItemErrorCode::SUCCESS
                                  : PrefetchItemErrorCode::IMPORT_ERROR));
  statement.BindInt64(2, offline_id);

  return statement.Run();
}

}  // namespace

ImportArchivesTask::ImportArchivesTask(PrefetchStore* prefetch_store,
                                       PrefetchImporter* prefetch_importer)
    : prefetch_store_(prefetch_store),
      prefetch_importer_(prefetch_importer),
      weak_ptr_factory_(this) {}

ImportArchivesTask::~ImportArchivesTask() {}

void ImportArchivesTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&GetPrefetchItemsSync),
      base::BindOnce(&ImportArchivesTask::OnPrefetchItemsRetrieved,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchivesTask::OnPrefetchItemsRetrieved(
    std::vector<PrefetchItem> items) {
  items_ = items;
  Process();
}

void ImportArchivesTask::Process() {
  if (items_.empty()) {
    TaskComplete();
    return;
  }

  PrefetchItem item(items_.back());
  items_.pop_back();

  prefetch_importer_->ImportFile(
      item, base::Bind(&ImportArchivesTask::OnArchiveImported,
                       weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchivesTask::OnArchiveImported(bool success, int64_t offline_id) {
  prefetch_store_->Execute(
      base::BindOnce(&UpdatePrefetchItemSync, offline_id, success),
      base::BindOnce(&ImportArchivesTask::OnItemUpdated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchivesTask::OnItemUpdated(bool success) {
  Process();
}

}  // namespace offline_pages
