// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/temporary_consistency_check_task.h"

#include <vector>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

// The define, struct and the MakePageInfo should be kept in sync on the fields.
#define PAGE_INFO_PROJECTION " offline_id, file_path"
struct PageInfo {
  int64_t offline_id;
  base::FilePath file_path;
};

PageInfo MakePageInfo(sql::Statement* statement) {
  PageInfo page_info;
  page_info.offline_id = statement->ColumnInt64(0);
  page_info.file_path =
      base::FilePath::FromUTF8Unsafe(statement->ColumnString(1));
  return page_info;
}

std::unique_ptr<std::vector<PageInfo>> GetAllTemporaryPageInfos(
    const std::vector<std::string>& temp_namespaces,
    sql::Connection* db) {
  auto result = base::MakeUnique<std::vector<PageInfo>>();

  const char kSql[] = "SELECT" PAGE_INFO_PROJECTION
                      " FROM offlinepages_v1"
                      " WHERE client_namespace = ?";
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return result;

  for (const auto& temp_namespace : temp_namespaces) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindString(0, temp_namespace);
    while (statement.Step())
      result->emplace_back(MakePageInfo(&statement));
  }

  if (!transaction.Commit())
    result->clear();
  return result;
}

std::set<base::FilePath> GetAllArchives(const base::FilePath& archives_dir) {
  std::set<base::FilePath> result;
  base::FileEnumerator file_enumerator(archives_dir, false,
                                       base::FileEnumerator::FILES);
  for (auto archive_path = file_enumerator.Next(); !archive_path.empty();
       archive_path = file_enumerator.Next()) {
    result.insert(archive_path);
  }
  return result;
}

bool DeletePagesByOfflineIds(const std::vector<int64_t>& offline_ids,
                             sql::Connection* db) {
  bool result = false;
  static const char kSql[] = "DELETE FROM offlinepages_v1 WHERE offline_id = ?";

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  for (const auto& offline_id : offline_ids) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindInt64(0, offline_id);
    result |= statement.Run();
  }

  if (!transaction.Commit())
    result = false;
  return result;
}

bool DeleteFiles(const std::vector<base::FilePath>& file_paths) {
  bool result = false;
  for (const auto& file_path : file_paths)
    result |= base::DeleteFile(file_path, false);
  return result;
}

bool CheckConsistency(const base::FilePath& archives_dir,
                      const base::FilePath& abandoned_dir,
                      const std::vector<std::string>& namespaces,
                      sql::Connection* db) {
  std::vector<int64_t> offline_ids_to_delete;
  std::vector<base::FilePath> files_to_delete;
  auto temp_page_infos = GetAllTemporaryPageInfos(namespaces, db);
  std::set<base::FilePath> archive_paths = GetAllArchives(archives_dir);

  for (const auto& page_info : *temp_page_infos) {
    // Get pages whose archive files are still in the abandoned directory.
    if (abandoned_dir.IsParent(page_info.file_path)) {
      offline_ids_to_delete.push_back(page_info.offline_id);
      files_to_delete.push_back(page_info.file_path);
      continue;
    }
    // Get pages whose archive files does not exist and delete.
    if (!base::PathExists(page_info.file_path)) {
      offline_ids_to_delete.push_back(page_info.offline_id);
      continue;
    }
  }

  for (const auto& archive_path : archive_paths) {
    if (std::find_if(temp_page_infos->begin(), temp_page_infos->end(),
                     [&archive_path](const PageInfo& page_info) -> bool {
                       return page_info.file_path == archive_path;
                     }) == temp_page_infos->end()) {
      files_to_delete.push_back(archive_path);
    }
  }

  DeletePagesByOfflineIds(offline_ids_to_delete, db);
  DeleteFiles(files_to_delete);
  return true;
}

}  // namespace

TemporaryConsistencyCheckTask::TemporaryConsistencyCheckTask(
    OfflinePageMetadataStoreSQL* store,
    ClientPolicyController* policy_controller,
    const base::FilePath& archives_dir,
    const base::FilePath& abandoned_dir)
    : store_(store),
      policy_controller_(policy_controller),
      archives_dir_(archives_dir),
      abandoned_dir_(abandoned_dir),
      weak_ptr_factory_(this) {}

TemporaryConsistencyCheckTask::~TemporaryConsistencyCheckTask() {}

void TemporaryConsistencyCheckTask::Run() {
  std::vector<std::string> temp_namespace_names =
      policy_controller_->GetNamespacesRemovedOnCacheReset();
  store_->Execute(
      base::BindOnce(&CheckConsistency, archives_dir_, abandoned_dir_,
                     temp_namespace_names),
      base::BindOnce(&TemporaryConsistencyCheckTask::OnCheckConsistencyDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TemporaryConsistencyCheckTask::OnCheckConsistencyDone(bool result) {
  TaskComplete();
}

}  // namespace offline_pages
