// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/delete_page_task.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "build/build_config.h"
#include "components/offline_pages/core/model/offline_store_utils.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

using DeletePageTaskResult = DeletePageTask::DeletePageTaskResult;
using PageInfo = DeletePageTask::PageInfo;

namespace {

bool DeleteArchiveSync(base::FilePath file_path) {
  // Delete the file only, |false| for recursive.
  return base::DeleteFile(file_path, false);
}

// Deletes a page from the store by |offline_id|.
bool DeletePageEntryByOfflineIdSync(sql::Connection* db, int64_t offline_id) {
  static const char kSql[] =
      "DELETE FROM " OFFLINE_PAGES_TABLE_NAME " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

// Deletes pages by PageInfo. This will return a DeletePageTaskResult which
// contains the PageInfos of the deleted pages (which are deleted from the disk
// and the store) and a DeletePageResult.
DeletePageTaskResult DeletePagesByPageInfosSync(
    sql::Connection* db,
    const std::vector<PageInfo>& page_infos) {
  std::vector<PageInfo> deleted_page_infos;

  // If there's no page to delete, return an empty list with SUCCESS.
  if (page_infos.size() == 0)
    return std::make_pair(deleted_page_infos, DeletePageResult::SUCCESS);

  bool any_archive_deleted = false;
  for (const auto& page_info : page_infos) {
    if (DeleteArchiveSync(page_info.file_path)) {
      any_archive_deleted = true;
      if (DeletePageEntryByOfflineIdSync(db, page_info.offline_id))
        deleted_page_infos.push_back(page_info);
    }
  }
  // If there're no files deleted, return DEVICE_FAILURE.
  if (!any_archive_deleted)
    return std::make_pair(deleted_page_infos, DeletePageResult::DEVICE_FAILURE);

  return std::make_pair(deleted_page_infos, DeletePageResult::SUCCESS);
}

// Gets the page info for |offline_id|, returning in |page_info|. Returns false
// if there's no record for |offline_id|.
bool GetPageInfoByOfflineIdSync(sql::Connection* db,
                                int64_t offline_id,
                                PageInfo* page_info) {
  static const char kSql[] =
      "SELECT client_namespace, client_id, file_path, request_origin"
      " FROM " OFFLINE_PAGES_TABLE_NAME " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);

  if (statement.Step()) {
    page_info->offline_id = offline_id;
    page_info->client_id.name_space = statement.ColumnString(0);
    page_info->client_id.id = statement.ColumnString(1);
    page_info->file_path =
        OfflineStoreUtils::GetPathFromUTF8String(statement.ColumnString(2));
    page_info->request_origin = statement.ColumnString(3);
    return true;
  }
  return false;
}

DeletePageTaskResult DeletePagesByOfflineIds(
    const std::vector<int64_t>& offline_ids,
    sql::Connection* db) {
  std::vector<PageInfo> infos;

  if (!db)
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);
  // TODO(romax): Make sure there's not regression to treat deleting empty list
  // of offline ids as successful (if it's not what we currently do).
  if (offline_ids.empty())
    return std::make_pair(infos, DeletePageResult::SUCCESS);

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);

  for (int64_t offline_id : offline_ids) {
    PageInfo info;
    if (GetPageInfoByOfflineIdSync(db, offline_id, &info))
      infos.push_back(info);
  }
  DeletePageTaskResult result = DeletePagesByPageInfosSync(db, infos);

  if (!transaction.Commit())
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);
  return result;
}

// Gets page infos for |client_id|, returning a vector of PageInfos because
// ClientId can refer to multiple pages.
std::vector<PageInfo> GetPageInfosByClientIdSync(sql::Connection* db,
                                                 ClientId client_id) {
  std::vector<PageInfo> page_infos;
  static const char kSql[] =
      "SELECT offline_id, file_path, request_origin"
      " FROM " OFFLINE_PAGES_TABLE_NAME
      " WHERE client_namespace = ? AND client_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, client_id.name_space);
  statement.BindString(1, client_id.id);

  while (statement.Step()) {
    PageInfo page_info;
    page_info.client_id = client_id;
    page_info.offline_id = statement.ColumnInt64(0);
    page_info.file_path =
        OfflineStoreUtils::GetPathFromUTF8String(statement.ColumnString(1));
    page_info.request_origin = statement.ColumnString(2);
    page_infos.push_back(page_info);
  }
  return page_infos;
}

DeletePageTaskResult DeletePagesByClientIds(
    const std::vector<ClientId> client_ids,
    sql::Connection* db) {
  std::vector<PageInfo> infos;

  if (!db)
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);
  // TODO(romax): Make sure there's not regression to treat deleting empty list
  // of offline ids as successful (if it's not what we currently do).
  if (client_ids.empty())
    return std::make_pair(infos, DeletePageResult::SUCCESS);

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);

  for (ClientId client_id : client_ids) {
    std::vector<PageInfo> temp_infos =
        GetPageInfosByClientIdSync(db, client_id);
    infos.insert(infos.end(), temp_infos.begin(), temp_infos.end());
  }

  DeletePageTaskResult result = DeletePagesByPageInfosSync(db, infos);

  if (!transaction.Commit())
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);
  return result;
}

// Gets the page infos of pages that satisfys |predicate|. Since UrlPredicate is
// passed in from outside of Offline Pages, it cannot be assumed to only check
// url equality. Hence getting the PageInfos of all pages.
std::vector<PageInfo> GetPageInfosByUrlPredicateSync(
    sql::Connection* db,
    const UrlPredicate& predicate) {
  std::vector<PageInfo> page_infos;
  static const char kSql[] =
      "SELECT offline_id, client_namespace, client_id, file_path, "
      "request_origin, online_url"
      " FROM " OFFLINE_PAGES_TABLE_NAME;
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));

  while (statement.Step()) {
    if (!predicate.Run(GURL(statement.ColumnString(5))))
      continue;
    PageInfo page_info;
    page_info.offline_id = statement.ColumnInt64(0);
    page_info.client_id.name_space = statement.ColumnString(1);
    page_info.client_id.id = statement.ColumnString(2);
    page_info.file_path =
        OfflineStoreUtils::GetPathFromUTF8String(statement.ColumnString(3));
    page_info.request_origin = statement.ColumnString(4);
    page_infos.push_back(page_info);
  }
  return page_infos;
}

DeletePageTaskResult DeletePagesByUrlPredicate(const UrlPredicate& predicate,
                                               sql::Connection* db) {
  std::vector<PageInfo> infos;
  if (!db)
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);

  infos = GetPageInfosByUrlPredicateSync(db, predicate);
  DeletePageTaskResult result = DeletePagesByPageInfosSync(db, infos);

  if (!transaction.Commit())
    return std::make_pair(infos, DeletePageResult::STORE_FAILURE);
  return result;
}

}  // namespace

PageInfo::PageInfo() = default;
PageInfo::PageInfo(const PageInfo& other) = default;

// static
std::unique_ptr<DeletePageTask> DeletePageTask::CreateTaskWithOfflineIds(
    OfflinePageMetadataStoreSQL* store,
    const std::vector<int64_t>& offline_ids,
    DeletePageTask::DeletePageTaskCallback callback) {
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store, base::BindOnce(&DeletePagesByOfflineIds, offline_ids),
      std::move(callback)));
}

// static
std::unique_ptr<DeletePageTask> DeletePageTask::CreateTaskWithClientIds(
    OfflinePageMetadataStoreSQL* store,
    const std::vector<ClientId>& client_ids,
    DeletePageTask::DeletePageTaskCallback callback) {
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store, base::BindOnce(&DeletePagesByClientIds, client_ids),
      std::move(callback)));
}

// static
std::unique_ptr<DeletePageTask> DeletePageTask::CreateTaskWithUrlPredicate(
    OfflinePageMetadataStoreSQL* store,
    const UrlPredicate& predicate,
    DeletePageTask::DeletePageTaskCallback callback) {
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store, base::BindOnce(&DeletePagesByUrlPredicate, predicate),
      std::move(callback)));
}

DeletePageTask::DeletePageTask(OfflinePageMetadataStoreSQL* store,
                               DeleteFunction func,
                               DeletePageTaskCallback callback)
    : store_(store),
      func_(std::move(func)),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {}

DeletePageTask::~DeletePageTask() {}

void DeletePageTask::Run() {
  DeletePages();
}

void DeletePageTask::DeletePages() {
  store_->Execute(std::move(func_),
                  base::BindOnce(&DeletePageTask::OnDeletePageDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void DeletePageTask::OnDeletePageDone(DeletePageTaskResult result) {
  std::vector<OfflinePageModel::DeletedPageInfo> deleted_page_info;
  for (const auto& page_info : result.first)
    deleted_page_info.push_back(OfflinePageModel::DeletedPageInfo(
        page_info.offline_id, page_info.client_id, page_info.request_origin));

  std::move(callback_).Run(deleted_page_info, result.second);
  TaskComplete();
}

}  // offline_pages
