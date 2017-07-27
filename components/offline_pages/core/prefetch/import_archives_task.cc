// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_archives_task.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

std::vector<PrefetchImporter::ArchiveInfo> GetArchivesSync(
    sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id, client_namespace, client_id, requested_url,"
      "  final_archived_url, title, file_path, file_size"
      " FROM prefetch_items"
      " WHERE state = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::DOWNLOADED));

  std::vector<PrefetchImporter::ArchiveInfo> result;
  while (statement.Step()) {
    PrefetchImporter::ArchiveInfo archive;
    archive.offline_id = statement.ColumnInt64(0);
    archive.client_id.name_space = statement.ColumnString(1);
    archive.client_id.id = statement.ColumnString(2);
    archive.url = GURL(statement.ColumnString(3));
    archive.final_archived_url = GURL(statement.ColumnString(4));
    archive.title = statement.ColumnString16(5);
    archive.file_path =
        base::FilePath::FromUTF8Unsafe(statement.ColumnString(6));
    archive.file_size = statement.ColumnInt64(7);
    result.push_back(archive);
  }

  return result;
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

bool UpdateToFinishedStateSync(int64_t offline_id,
                               bool success,
                               sql::Connection* db) {
  static const char kSql[] =
      "UPDATE prefetch_items"
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
      base::BindOnce(&GetArchivesSync),
      base::BindOnce(&ImportArchivesTask::OnArchivesRetrieved,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchivesTask::OnArchivesRetrieved(
    std::vector<PrefetchImporter::ArchiveInfo> archives) {
  archives_ = archives;
  Process();
}

void ImportArchivesTask::Process() {
  if (archives_.empty()) {
    TaskComplete();
    return;
  }

  PrefetchImporter::ArchiveInfo archive = archives_.back();
  archives_.pop_back();

  prefetch_store_->Execute(
      base::BindOnce(&UpdateToImportingStateSync, archive.offline_id),
      base::BindOnce(&ImportArchivesTask::OnStateUpdatedToImporting,
                     weak_ptr_factory_.GetWeakPtr(), archive));
}

void ImportArchivesTask::OnStateUpdatedToImporting(
    const PrefetchImporter::ArchiveInfo& archive,
    bool success) {
  // If the state fails to be updated, skip importing this one.
  if (!success) {
    Process();
    return;
  }

  prefetch_importer_->ImportArchive(
      archive, base::Bind(&ImportArchivesTask::OnArchiveImported,
                          weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchivesTask::OnArchiveImported(bool success, int64_t offline_id) {
  prefetch_store_->Execute(
      base::BindOnce(&UpdateToFinishedStateSync, offline_id, success),
      base::BindOnce(&ImportArchivesTask::OnStateUpdatedToFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ImportArchivesTask::OnStateUpdatedToFinished(bool success) {
  Process();
}

}  // namespace offline_pages
