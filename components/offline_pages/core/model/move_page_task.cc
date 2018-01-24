// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/move_page_task.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

namespace offline_pages {

namespace {

// Synchonously do the file move and run the DB update. Returns true for
// success.
MovePageTaskDisposition ChangeFileLocationSync(
    int64_t offline_id,
    const base::FilePath& existing_file_path,
    const base::FilePath& destination_file_path,
    sql::Connection* db) {
  if (!db)
    return MovePageTaskDisposition::MOVE_FAILED;

  // Move the file to the new location.
  bool moved = base::Move(existing_file_path, destination_file_path);
  if (!moved) {
    DVLOG(1) << "Unable to move file " << existing_file_path.value() << " to "
             << destination_file_path.value() << " " << __func__;
    return MovePageTaskDisposition::MOVE_FAILED;
  }

  // Update the location in the OPM database.
  const char kSql[] = "UPDATE OR IGNORE " OFFLINE_PAGES_TABLE_NAME
                      " SET file_path = ?"
                      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0,
                       store_utils::ToDatabaseFilePath(destination_file_path));
  statement.BindInt64(1, offline_id);
  if (statement.Run())
    return MovePageTaskDisposition::SUCCESS;
  else
    return MovePageTaskDisposition::DB_WRITE_FAILED;
}

}  // namespace

MovePageTask::MovePageTask(OfflinePageMetadataStoreSQL* store,
                           int64_t offline_id,
                           const base::FilePath& existing_file_path,
                           const base::FilePath& destination_file_path,
                           MovePageCallback callback)
    : store_(store),
      offline_id_(offline_id),
      existing_file_path_(existing_file_path),
      destination_file_path_(destination_file_path),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {}

MovePageTask::~MovePageTask() {}

void MovePageTask::Run() {
  if (destination_file_path_.empty()) {
    OnMovePageDone(MovePageTaskDisposition::MOVE_FAILED);
    return;
  }

  // Update the file location in the OfflinePageModel database, then call the
  // callback.
  store_->Execute(base::BindOnce(&ChangeFileLocationSync, offline_id_,
                                 existing_file_path_, destination_file_path_),
                  base::BindOnce(&MovePageTask::OnMovePageDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void MovePageTask::OnMovePageDone(MovePageTaskDisposition result) {
  TaskComplete();

  std::move(callback_).Run(result, destination_file_path_);
}

}  // namespace offline_pages
