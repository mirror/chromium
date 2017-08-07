// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_archive_task.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

namespace offline_pages {
namespace {

PrefetchArchiveInfo GetArchiveSync(sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id, client_namespace, client_id, requested_url,"
      "  final_archived_url, title, file_path, file_size"
      " FROM prefetch_items"
      " WHERE state = ?"
      " LIMIT 1";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADED));

  PrefetchArchiveInfo archive;
  if (statement.Step()) {
    archive.offline_id = statement.ColumnInt64(0);
    archive.client_id.name_space = statement.ColumnString(1);
    archive.client_id.id = statement.ColumnString(2);
    archive.url = GURL(statement.ColumnString(3));
    archive.final_archived_url = GURL(statement.ColumnString(4));
    archive.title = statement.ColumnString16(5);
    archive.file_path =
        base::FilePath::FromUTF8Unsafe(statement.ColumnString(6));
    archive.file_size = statement.ColumnInt64(7);
  }

  return archive;
}

bool UpdateToImportingStateSync(int64_t offline_id, sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?"
      " WHERE offline_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::IMPORTING));
  statement.BindInt64(1, offline_id);

  return statement.Run();
}

PrefetchArchiveInfo GetArchiveAndUpdateToImportingStateSync(
    sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return PrefetchArchiveInfo();

  PrefetchArchiveInfo archive = GetArchiveSync(db);
  if (archive.empty())
    return PrefetchArchiveInfo();

  if (!UpdateToImportingStateSync(archive.offline_id, db))
    return PrefetchArchiveInfo();

  if (!transaction.Commit())
    return PrefetchArchiveInfo();

  return archive;
}

}  // namespace

ImportArchiveTask::ImportArchiveTask(PrefetchStore* prefetch_store,
                                     PrefetchImporter* prefetch_importer)
    : prefetch_store_(prefetch_store),
      prefetch_importer_(prefetch_importer),
      weak_ptr_factory_(this) {}

ImportArchiveTask::~ImportArchiveTask() {}

void ImportArchiveTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&GetArchiveAndUpdateToImportingStateSync),
      base::BindOnce(&ImportArchiveTask::OnArchiveRetrieved,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchiveTask::OnArchiveRetrieved(PrefetchArchiveInfo archive) {
  if (!archive.empty())
    prefetch_importer_->ImportArchive(archive);

  TaskComplete();
}

}  // namespace offline_pages
