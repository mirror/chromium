// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_cleanup_task.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "components/offline_pages/core/prefetch/prefetch_importer.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

std::vector<int64_t> GetAllOngoingImportsSync(sql::Connection* db) {
  static const char kSql[] =
      "SELECT offline_id"
      " FROM prefetch_items"
      " WHERE state = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::IMPORTING));

  std::vector<int64_t> offline_ids;
  while (statement.Step())
    offline_ids.push_back(statement.ColumnInt64(0));
  return offline_ids;
}

bool ExpireImportSync(int64_t offline_id, sql::Connection* db) {
  if (!db)
    return false;

  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?, error_code = ?"
      " WHERE state = ? AND offline_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(1, static_cast<int>(PrefetchItemErrorCode::IMPORT_ABORTED));
  statement.BindInt(2, static_cast<int>(PrefetchItemState::IMPORTING));
  statement.BindInt64(3, offline_id);

  return statement.Run();
}

bool CleanupImportsSync(std::list<int64_t> importing_offline_ids,
                        sql::Connection* db) {
  if (!db)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  std::vector<int64_t> importing_offline_ids_in_store =
      GetAllOngoingImportsSync(db);
  if (importing_offline_ids_in_store.empty())
    return true;

  for (const auto& importing_offline_id_in_store :
       importing_offline_ids_in_store) {
    // If the import is in progress, no change.
    if (std::find(importing_offline_ids.begin(), importing_offline_ids.end(),
                  importing_offline_id_in_store) !=
        importing_offline_ids.end()) {
      continue;
    }

    if (!ExpireImportSync(importing_offline_id_in_store, db))
      return false;
  }

  return transaction.Commit();
}

}  // namespace

ImportCleanupTask::ImportCleanupTask(PrefetchStore* prefetch_store,
                                     PrefetchImporter* prefetch_importer)
    : prefetch_store_(prefetch_store),
      prefetch_importer_(prefetch_importer),
      weak_ptr_factory_(this) {}

ImportCleanupTask::~ImportCleanupTask() {}

void ImportCleanupTask::Run() {
  std::list<int64_t> importing_offline_ids_ =
      prefetch_importer_->GetOngoingImports();
  prefetch_store_->Execute(
      base::BindOnce(&CleanupImportsSync, importing_offline_ids_),
      base::BindOnce(&ImportCleanupTask::OnFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ImportCleanupTask::OnFinished(bool success) {
  TaskComplete();
}

}  // namespace offline_pages
