// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/tasks/delete_page_task.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/offline_pages/core/archive_manager.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

namespace {

base::FilePath GetPathFromUTF8String(const std::string& path_string) {
#if defined(OS_POSIX)
  return base::FilePath(path_string);
#elif defined(OS_WIN)
  return base::FilePath(base::UTF8ToWide(path_string));
#else
#error Unknown OS
#endif
}

std::unique_ptr<OfflinePagesUpdateResult> BuildUpdateResultForIds(
    StoreState store_state,
    const std::vector<int64_t>& offline_ids,
    ItemActionStatus action_status) {
  std::unique_ptr<OfflinePagesUpdateResult> result(
      new OfflinePagesUpdateResult(store_state));
  for (const auto& offline_id : offline_ids)
    result->item_statuses.push_back(std::make_pair(offline_id, action_status));
  return result;
}

// Create an offline page item from a SQL result.  Expects complete rows with
// all columns present.
OfflinePageItem MakeOfflinePageItem(sql::Statement* statement) {
  int64_t id = statement->ColumnInt64(0);
  base::Time creation_time = FromDatabaseTime(statement->ColumnInt64(1));
  int64_t file_size = statement->ColumnInt64(2);
  base::Time last_access_time = FromDatabaseTime(statement->ColumnInt64(3));
  int access_count = statement->ColumnInt(4);
  int64_t system_download_id = statement->ColumnInt64(5);
  base::Time file_missing_time = FromDatabaseTime(statement->ColumnInt64(6));
  int upgrade_attempt = statement->ColumnInt(7);
  ClientId client_id(statement->ColumnString(8), statement->ColumnString(9));
  GURL url(statement->ColumnString(10));
  base::FilePath path(GetPathFromUTF8String(statement->ColumnString(11)));
  base::string16 title = statement->ColumnString16(12);
  GURL original_url(statement->ColumnString(13));
  std::string request_origin = statement->ColumnString(14);
  std::string digest = statement->ColumnString(15);

  OfflinePageItem item(url, id, client_id, path, file_size, creation_time);
  item.last_access_time = last_access_time;
  item.access_count = access_count;
  item.title = title;
  item.original_url = original_url;
  item.request_origin = request_origin;
  item.system_download_id = system_download_id;
  item.file_missing_time = file_missing_time;
  item.upgrade_attempt = upgrade_attempt;
  item.digest = digest;
  return item;
}

std::vector<OfflinePageItem> GetAllPagesSync(sql::Connection* db) {
  std::vector<OfflinePageItem> result;
  const char kSql[] = "SELECT * FROM " OFFLINE_PAGES_TABLE_NAME;
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));

  while (statement.Step()) {
    result.push_back(MakeOfflinePageItem(&statement));
  }
  return result;
}

bool GetPageByOfflineIdSync(sql::Connection* db,
                            int64_t offline_id,
                            OfflinePageItem* page) {
  DCHECK(page);
  const char kSql[] =
      "SELECT * FROM " OFFLINE_PAGES_TABLE_NAME " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);

  if (statement.Step()) {
    *page = MakeOfflinePageItem(&statement);
    return true;
  }
  return false;
}

bool DeleteByOfflineIdSync(sql::Connection* db, int64_t offline_id) {
  static const char kSql[] =
      "DELETE FROM " OFFLINE_PAGES_TABLE_NAME " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

std::unique_ptr<OfflinePagesUpdateResult> DeletePagesByOfflineIds(
    const std::vector<int64_t>& offline_ids,
    sql::Connection* db) {
  if (!db) {
    return BuildUpdateResultForIds(StoreState::NOT_LOADED, offline_ids,
                                   ItemActionStatus::STORE_ERROR);
  }

  // TODO(romax): Make sure there's not regression to treat deleting empty list
  // of offline ids as successful (if it's not what we currently do).
  if (offline_ids.empty()) {
    return BuildUpdateResultForIds(StoreState::LOADED, offline_ids,
                                   ItemActionStatus::SUCCESS);
  }

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return BuildUpdateResultForIds(StoreState::LOADED, offline_ids,
                                   ItemActionStatus::STORE_ERROR);
  }

  std::unique_ptr<OfflinePagesUpdateResult> result(
      new OfflinePagesUpdateResult(StoreState::LOADED));

  for (int64_t offline_id : offline_ids) {
    OfflinePageItem page;
    ItemActionStatus status;
    if (!GetPageByOfflineIdSync(db, offline_id, &page)) {
      status = ItemActionStatus::NOT_FOUND;
    } else if (!DeleteByOfflineIdSync(db, offline_id)) {
      status = ItemActionStatus::STORE_ERROR;
    } else {
      status = ItemActionStatus::SUCCESS;
      result->updated_items.push_back(page);
    }
    result->item_statuses.push_back(std::make_pair(offline_id, status));
  }

  if (!transaction.Commit()) {
    return BuildUpdateResultForIds(StoreState::LOADED, offline_ids,
                                   ItemActionStatus::STORE_ERROR);
  }

  return result;
}

std::vector<int64_t> GetOfflineIdByClientIdSync(ClientId client_id,
                                                sql::Connection* db) {
  static const char kSql[] = "SELECT offline_id FROM " OFFLINE_PAGES_TABLE_NAME
                             " WHERE client_namespace = ? AND client_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, client_id.name_space);
  statement.BindString(1, client_id.id);

  std::vector<int64_t> result;
  while (statement.Step())
    result.push_back(statement.ColumnInt64(0));
  return result;
}

std::vector<int64_t> GetOfflineIdsFromClientIds(
    const std::vector<ClientId> client_ids,
    sql::Connection* db) {
  std::vector<int64_t> result;
  for (const auto& client_id : client_ids) {
    const std::vector<int64_t>& offline_ids =
        GetOfflineIdByClientIdSync(client_id, db);
    // A result > 0 means the offline id was correctly found.
    if (offline_ids.size() > 0)
      result.insert(result.end(), offline_ids.begin(), offline_ids.end());
  }
  return result;
}

std::unique_ptr<OfflinePagesUpdateResult> DeletePagesByClientIds(
    const std::vector<ClientId> client_ids,
    sql::Connection* db) {
  std::vector<int64_t> offline_ids = GetOfflineIdsFromClientIds(client_ids, db);
  return DeletePagesByOfflineIds(offline_ids, db);
}

std::vector<int64_t> GetOfflineIdsFromPagePredicate(
    const PagePredicate& predicate,
    sql::Connection* db) {
  std::vector<int64_t> result;
  const std::vector<OfflinePageItem>& pages = GetAllPagesSync(db);
  for (const auto& page : pages) {
    if (predicate.Run(page))
      result.push_back(page.offline_id);
  }
  return result;
}

std::unique_ptr<OfflinePagesUpdateResult> DeletePagesByPagePredicate(
    const PagePredicate& predicate,
    sql::Connection* db) {
  std::vector<int64_t> offline_ids =
      GetOfflineIdsFromPagePredicate(predicate, db);
  return DeletePagesByOfflineIds(offline_ids, db);
}

}  // namespace

// static
std::unique_ptr<DeletePageTask> DeletePageTaskFactory::CreateTaskWithOfflineIds(
    OfflinePageMetadataStoreSQL* store,
    ArchiveManager* archive_manager,
    const std::vector<int64_t>& offline_ids,
    const DeletePageTask::DeletePageTaskCallback& callback) {
  return base::MakeUnique<DeletePageTask>(
      store, archive_manager,
      base::BindOnce(&DeletePagesByOfflineIds, offline_ids), callback);
}

// static
std::unique_ptr<DeletePageTask> DeletePageTaskFactory::CreateTaskWithClientIds(
    OfflinePageMetadataStoreSQL* store,
    ArchiveManager* archive_manager,
    const std::vector<ClientId>& client_ids,
    const DeletePageTask::DeletePageTaskCallback& callback) {
  return base::MakeUnique<DeletePageTask>(
      store, archive_manager,
      base::BindOnce(&DeletePagesByClientIds, client_ids), callback);
}

// static
std::unique_ptr<DeletePageTask>
DeletePageTaskFactory::CreateTaskWithPagePredicate(
    OfflinePageMetadataStoreSQL* store,
    ArchiveManager* archive_manager,
    const PagePredicate& predicate,
    const DeletePageTask::DeletePageTaskCallback& callback) {
  return base::MakeUnique<DeletePageTask>(
      store, archive_manager,
      base::BindOnce(&DeletePagesByPagePredicate, predicate), callback);
}

DeletePageTask::DeletePageTask(OfflinePageMetadataStoreSQL* store,
                               ArchiveManager* archive_manager,
                               DeleteFunction func,
                               const DeletePageTaskCallback& callback)
    : store_(store),
      archive_manager_(archive_manager),
      func_(std::move(func)),
      callback_(callback),
      weak_ptr_factory_(this) {}

DeletePageTask::~DeletePageTask() {}

void DeletePageTask::Run() {
  store_->Execute(std::move(func_),
                  base::BindOnce(&DeletePageTask::OnDeletePageDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void DeletePageTask::OnDeletePageDone(
    std::unique_ptr<OfflinePagesUpdateResult> result) {
  std::vector<base::FilePath> paths_to_delete;
  for (const auto& page : result->updated_items)
    paths_to_delete.push_back(page.file_path);

  // If there's no pages deleted in the metadata database, there should be no
  // archive files to be deleted as well. So calling the callback manually with
  // a result of success.
  if (paths_to_delete.empty()) {
    OnDeleteArchiveFilesDone(std::move(result), true);
    return;
  }

  archive_manager_->DeleteMultipleArchives(
      paths_to_delete, base::Bind(&DeletePageTask::OnDeleteArchiveFilesDone,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  base::Passed(std::move(result))));
}

void DeletePageTask::OnDeleteArchiveFilesDone(
    std::unique_ptr<OfflinePagesUpdateResult> result,
    bool delete_files_result) {
  DeletePageResult delete_result;
  if (result->store_state == StoreState::LOADED)
    delete_result = DeletePageResult::SUCCESS;
  else
    delete_result = DeletePageResult::STORE_FAILURE;
  if (!delete_files_result)
    delete_result = DeletePageResult::DEVICE_FAILURE;
  callback_.Run(result->updated_items, delete_result);
  TaskComplete();
}

}  // offline_pages
