// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/get_pages_task.h"

#include <algorithm>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

namespace offline_pages {
namespace {

using ReadResult = GetPagesTask::ReadResult;

#define OFFLINE_PAGE_PROJECTION                                \
  " offline_id, creation_time, file_size, last_access_time,"   \
  " access_count, system_download_id, file_missing_time,"      \
  " upgrade_attempt, client_namespace, client_id, online_url," \
  " file_path, title, original_url, request_origin, digest"

base::FilePath GetPathFromUTF8String(const std::string& path_string) {
#if defined(OS_POSIX)
  return base::FilePath(path_string);
#elif defined(OS_WIN)
  return base::FilePath(base::UTF8ToWide(path_string));
#else
#error Unknown OS
#endif
}

// Create an offline page item from a SQL result.
// Expects the order of columns as defined by OFFLINE_PAGE_PROJECTION macro.
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

ReadResult ReadAllPagesSync(sql::Connection* db) {
  ReadResult result;
  if (!db) {
    result.success = false;
    return result;
  }

  static const char kSql[] =
      "SELECT " OFFLINE_PAGE_PROJECTION " FROM offlinepages_v1";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  while (statement.Step())
    result.pages.emplace_back(MakeOfflinePageItem(&statement));

  result.success = true;
  return result;
}

ReadResult ReadPagesByClientIdsSync(const std::vector<ClientId>& client_ids,
                                    sql::Connection* db) {
  ReadResult result;
  if (!db) {
    result.success = false;
    return result;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return result;

  static const char kSql[] = "SELECT " OFFLINE_PAGE_PROJECTION
                             " FROM offlinepages_v1"
                             " WHERE client_namespace = ? AND client_id = ?";
  for (auto client_id : client_ids) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindString(0, client_id.name_space);
    statement.BindString(1, client_id.id);
    while (statement.Step())
      result.pages.emplace_back(MakeOfflinePageItem(&statement));
  }

  if (!transaction.Commit()) {
    result.pages.clear();
    return result;
  }

  result.success = true;
  return result;
}

ReadResult ReadPagesByRequestOriginSync(const std::string& request_origin,
                                        sql::Connection* db) {
  ReadResult result;
  if (!db) {
    result.success = false;
    return result;
  }

  static const char kSql[] = "SELECT " OFFLINE_PAGE_PROJECTION
                             " FROM offlinepages_v1"
                             " WHERE request_origin = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, request_origin);
  while (statement.Step())
    result.pages.emplace_back(MakeOfflinePageItem(&statement));

  result.success = true;
  return result;
}

ReadResult ReadPagesByUrlSync(const GURL& url, sql::Connection* db) {
  ReadResult result;
  if (!db) {
    result.success = false;
    return result;
  }

  // Because we are using SQLite's like function, there is some magic necessary
  // to simplify pattern matching => first replace % with _ giving it a single
  // character wildcard match, followed by appending % to match fragments in
  // bulk at the end. False positives are dropped in the while loop.
  GURL::Replacements remove_fragment;
  remove_fragment.ClearRef();
  GURL url_to_match = url.ReplaceComponents(remove_fragment);
  std::string string_to_match = url_to_match.spec();
  std::replace(string_to_match.begin(), string_to_match.end(), '%', '_');
  string_to_match = string_to_match.append(1UL, '%');
  static const char kSql[] =
      "SELECT " OFFLINE_PAGE_PROJECTION
      " FROM offlinepages_v1"
      " WHERE like(?, online_url) OR like(?, original_url)";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, string_to_match);
  statement.BindString(1, string_to_match);
  while (statement.Step()) {
    OfflinePageItem temp_item = MakeOfflinePageItem(&statement);
    if (temp_item.url.ReplaceComponents(remove_fragment) == url_to_match ||
        temp_item.original_url.ReplaceComponents(remove_fragment) ==
            url_to_match) {
      result.pages.push_back(temp_item);
    }
  }

  result.success = true;
  return result;
}

ReadResult ReadPagesByOfflineId(int64_t offline_id, sql::Connection* db) {
  ReadResult result;
  if (!db) {
    result.success = false;
    return result;
  }

  static const char kSql[] = "SELECT " OFFLINE_PAGE_PROJECTION
                             " FROM offlinepages_v1"
                             " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  while (statement.Step())
    result.pages.emplace_back(MakeOfflinePageItem(&statement));

  result.success = true;
  return result;
}

void WrapInMultipleItemsCallback(const SingleOfflinePageItemCallback& callback,
                                 const std::vector<OfflinePageItem>& pages) {
  if (pages.size() == 0)
    callback.Run(nullptr);
  else
    callback.Run(&pages[0]);
}

}  // namespace

GetPagesTask::ReadResult::ReadResult() : success(false) {}

GetPagesTask::ReadResult::ReadResult(const ReadResult& other)
    : success(other.success), pages(other.pages) {}

GetPagesTask::ReadResult::~ReadResult() {}

// static
std::unique_ptr<GetPagesTask> GetPagesTask::CreateTaskForAllPages(
    OfflinePageMetadataStoreSQL* store,
    const MultipleOfflinePageItemCallback& callback) {
  return std::unique_ptr<GetPagesTask>(
      new GetPagesTask(store, base::BindOnce(&ReadAllPagesSync), callback));
}

// static
std::unique_ptr<GetPagesTask> GetPagesTask::CreateTaskForClientIds(
    OfflinePageMetadataStoreSQL* store,
    const MultipleOfflinePageItemCallback& callback,
    const std::vector<ClientId>& client_ids) {
  // Creates an instance of GetPagesTask, which wraps the client_ids argument in
  // a OnceClosure. It will then be used to execute.
  return std::unique_ptr<GetPagesTask>(new GetPagesTask(
      store, base::BindOnce(&ReadPagesByClientIdsSync, client_ids), callback));
}

// static
std::unique_ptr<GetPagesTask> GetPagesTask::CreateTaskForRequestOrigin(
    OfflinePageMetadataStoreSQL* store,
    const MultipleOfflinePageItemCallback& callback,
    const std::string& request_origin) {
  return std::unique_ptr<GetPagesTask>(new GetPagesTask(
      store, base::BindOnce(&ReadPagesByRequestOriginSync, request_origin),
      callback));
}

// static
std::unique_ptr<GetPagesTask> GetPagesTask::CreateTaskForUrl(
    OfflinePageMetadataStoreSQL* store,
    const MultipleOfflinePageItemCallback& callback,
    const GURL& url) {
  return std::unique_ptr<GetPagesTask>(new GetPagesTask(
      store, base::BindOnce(&ReadPagesByUrlSync, url), callback));
}

// static
std::unique_ptr<GetPagesTask> GetPagesTask::CreateTaskForOfflineId(
    OfflinePageMetadataStoreSQL* store,
    const SingleOfflinePageItemCallback& callback,
    int64_t offline_id) {
  return std::unique_ptr<GetPagesTask>(
      new GetPagesTask(store, base::BindOnce(&ReadPagesByOfflineId, offline_id),
                       base::Bind(&WrapInMultipleItemsCallback, callback)));
}

GetPagesTask::GetPagesTask(OfflinePageMetadataStoreSQL* store,
                           DbWorkCallback db_work_callback,
                           const MultipleOfflinePageItemCallback& callback)
    : store_(store),
      db_work_callback_(std::move(db_work_callback)),
      callback_(callback),
      weak_ptr_factory_(this) {
  DCHECK(store_);
  DCHECK(!callback_.is_null());
}

GetPagesTask::~GetPagesTask() {}

void GetPagesTask::Run() {
  ReadRequests();
}

void GetPagesTask::ReadRequests() {
  store_->Execute(std::move(db_work_callback_),
                  base::BindOnce(&GetPagesTask::CompleteWithResult,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void GetPagesTask::CompleteWithResult(ReadResult result) {
  callback_.Run(result.pages);
  TaskComplete();
}

}  // namespace offline_pages
