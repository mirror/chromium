// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/tasks/add_page_task.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "components/offline_pages/core/task.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

namespace {

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

std::string GetUTF8StringFromPath(const base::FilePath& path) {
#if defined(OS_POSIX)
  return path.value();
#elif defined(OS_WIN)
  return base::WideToUTF8(path.value());
#else
#error Unknown OS
#endif
}

ItemActionStatus AddOfflinePageSync(const OfflinePageItem& item,
                                    sql::Connection* db) {
  if (!db)
    return ItemActionStatus::STORE_ERROR;

  static const char kSql[] =
      "INSERT OR IGNORE INTO " OFFLINE_PAGES_TABLE_NAME
      " (offline_id, online_url, client_namespace, client_id, file_path, "
      "file_size, creation_time, last_access_time, access_count, "
      "title, original_url, request_origin, system_download_id, "
      "file_missing_time, upgrade_attempt, digest)"
      " VALUES "
      " (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, item.offline_id);
  statement.BindString(1, item.url.spec());
  statement.BindString(2, item.client_id.name_space);
  statement.BindString(3, item.client_id.id);
  statement.BindString(4, GetUTF8StringFromPath(item.file_path));
  statement.BindInt64(5, item.file_size);
  statement.BindInt64(6, ToDatabaseTime(item.creation_time));
  statement.BindInt64(7, ToDatabaseTime(item.last_access_time));
  statement.BindInt(8, item.access_count);
  statement.BindString16(9, item.title);
  statement.BindString(10, item.original_url.spec());
  statement.BindString(11, item.request_origin);
  statement.BindInt64(12, item.system_download_id);
  statement.BindInt64(13, ToDatabaseTime(item.file_missing_time));
  statement.BindInt(14, item.upgrade_attempt);
  statement.BindString(15, item.digest);

  if (!statement.Run())
    return ItemActionStatus::STORE_ERROR;
  if (db->GetLastChangeCount() == 0)
    return ItemActionStatus::ALREADY_EXISTS;
  return ItemActionStatus::SUCCESS;
}

}  // namespace

AddPageTask::AddPageTask(OfflinePageMetadataStoreSQL* store,
                         const OfflinePageItem& offline_page,
                         const AddPageTaskCallback& callback)
    : store_(store),
      offline_page_(offline_page),
      callback_(callback),
      weak_ptr_factory(this) {}

AddPageTask::~AddPageTask() {}

void AddPageTask::Run() {
  if (!store_) {
    InformAddPageDone(AddPageResult::STORE_FAILURE);
    return;
  }
  store_->Execute(base::BindOnce(&AddOfflinePageSync, offline_page_),
                  base::BindOnce(&AddPageTask::OnAddPageDone,
                                 weak_ptr_factory.GetWeakPtr()));
}

void AddPageTask::OnAddPageDone(ItemActionStatus status) {
  AddPageResult result;
  switch (status) {
    case ItemActionStatus::SUCCESS:
      result = AddPageResult::SUCCESS;
      break;
    case ItemActionStatus::ALREADY_EXISTS:
      result = AddPageResult::ALREADY_EXISTS;
      break;
    default:
      result = AddPageResult::STORE_FAILURE;
  }
  InformAddPageDone(result);
}

void AddPageTask::InformAddPageDone(AddPageResult result) {
  callback_.Run(result, offline_page_);
  TaskComplete();
}

}  // namespace offline_pages
