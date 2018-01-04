// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/persistent_pages_consistency_check_task.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

struct PageInfo {
  int64_t offline_id;
  base::FilePath file_path;
};

std::vector<PageInfo> GetAllPersistentPageInfos(
    const std::vector<std::string>& namespaces,
    sql::Connection* db) {
  std::vector<PageInfo> result;

  const char kSql[] =
      "SELECT offline_id, file_path"
      " FROM " OFFLINE_PAGES_TABLE_NAME " WHERE client_namespace = ?";

  for (const auto& persistent_namespace : namespaces) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindString(0, persistent_namespace);
    while (statement.Step())
      result.push_back(
          {statement.ColumnInt64(0),
           store_utils::FromDatabaseFilePath(statement.ColumnString(1))});
  }

  return result;
}

bool DeletePagesByOfflineIds(const std::vector<int64_t>& offline_ids,
                             sql::Connection* db) {
  static const char kSql[] =
      "DELETE FROM " OFFLINE_PAGES_TABLE_NAME " WHERE offline_id = ?";

  for (const auto& offline_id : offline_ids) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindInt64(0, offline_id);
    if (!statement.Run())
      return false;
  }
  return true;
}

SyncOperationResult CheckConsistencySync(
    const base::FilePath& archives_dir,
    const std::vector<std::string>& namespaces,
    sql::Connection* db) {
  if (!db)
    return SyncOperationResult::INVALID_DB_CONNECTION;

  // One large database transaction that will:
  // 1. Gets persistent page infos from the database.
  // 2. Decide which pages to delete.
  // 3. Delete metadata entries from the database.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return SyncOperationResult::TRANSACTION_BEGIN_ERROR;

  auto persistent_page_infos = GetAllPersistentPageInfos(namespaces, db);

  std::set<base::FilePath> persistent_page_info_paths;
  std::vector<int64_t> offline_ids_to_delete;
  for (const auto& page_info : persistent_page_infos) {
    // Get pages whose archive files does not exist and delete.
    if (!base::PathExists(page_info.file_path)) {
      offline_ids_to_delete.push_back(page_info.offline_id);
      continue;
    }
    // Extract existing file paths from |temp_page_infos| so that we can do a
    // faster matching later.
    persistent_page_info_paths.insert(page_info.file_path);
  }
  // Try to delete the pages by offline ids collected above.
  // If there's any database related errors, the function will return false,
  // and the database operations will be rolled back since the transaction will
  // not be committed.
  if (!DeletePagesByOfflineIds(offline_ids_to_delete, db))
    return SyncOperationResult::DB_OPERATION_ERROR;

  if (offline_ids_to_delete.size() > 0) {
    UMA_HISTOGRAM_COUNTS(
        "OfflinePages.ConsistencyCheck.Persistent.PagesMissingArchiveFileCount",
        static_cast<int32_t>(offline_ids_to_delete.size()));
  }

  if (!transaction.Commit())
    return SyncOperationResult::TRANSACTION_COMMIT_ERROR;

  return SyncOperationResult::SUCCESS;
}

}  // namespace

PersistentPagesConsistencyCheckTask::PersistentPagesConsistencyCheckTask(
    OfflinePageMetadataStoreSQL* store,
    ClientPolicyController* policy_controller,
    const base::FilePath& archives_dir)
    : store_(store),
      policy_controller_(policy_controller),
      archives_dir_(archives_dir),
      weak_ptr_factory_(this) {
  DCHECK(store_);
  DCHECK(policy_controller_);
}

PersistentPagesConsistencyCheckTask::~PersistentPagesConsistencyCheckTask() {}

void PersistentPagesConsistencyCheckTask::Run() {
  std::vector<std::string> namespaces = policy_controller_->GetAllNamespaces();
  std::vector<std::string> persistent_namespace_names;
  for (const auto& name_space : namespaces) {
    if (!policy_controller_->IsRemovedOnCacheReset(name_space))
      persistent_namespace_names.push_back(name_space);
  }
  store_->Execute(
      base::BindOnce(&CheckConsistencySync, archives_dir_,
                     persistent_namespace_names),
      base::BindOnce(
          &PersistentPagesConsistencyCheckTask::OnCheckConsistencyDone,
          weak_ptr_factory_.GetWeakPtr()));
}

void PersistentPagesConsistencyCheckTask::OnCheckConsistencyDone(
    SyncOperationResult result) {
  UMA_HISTOGRAM_ENUMERATION("OfflinePages.ConsistencyCheck.Persistent.Result",
                            result, SyncOperationResult::RESULT_COUNT);
  TaskComplete();
}

}  // namespace offline_pages
