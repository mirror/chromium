// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/move_page_task.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "sql/connection.h"
#include "sql/statement.h"

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

namespace offline_pages {

namespace {

// Synchonously do the file move and run the DB update. Returns true for
// success.
bool ChangeFileLocationSync(const OfflinePageItem& page,
                            const base::FilePath new_file_path,
                            sql::Connection* db) {
  if (!db)
    return false;

  // Move the file to the new location.
  bool moved = base::Move(page.file_path, new_file_path);
  if (!moved) {
    DVLOG(1) << "Unable to move file " << page.file_path.value() << " to "
             << new_file_path.value() << " " << __func__;
    return false;
  }

  // If the move succeeds, but the DB update fails, then the page will still be
  // present on the device, but Downloads Home won't be able to find it.

  const char kSql[] = "UPDATE OR IGNORE " OFFLINE_PAGES_TABLE_NAME
                      " SET file_path = ?"
                      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, new_file_path.value());
  statement.BindInt64(1, page.offline_id);
  statement.Run();
  // We return success if we moved the file, even if the DB update failed.
  return moved;
}

}  // namespace

MovePageTask::MovePageTask(OfflinePageMetadataStoreSQL* store,
                           const OfflinePageItem& page,
                           const base::FilePath& public_directory,
                           MovePageCallback callback)
    : store_(store),
      page_(page),
      public_directory_(public_directory),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {}

MovePageTask::~MovePageTask() {}

void MovePageTask::Run() {
  if (public_directory_.empty()) {
    OnMovePageDone(false);
    return;
  }
  // Construct full file path in the new directory.
  base::FilePath base_name = page_.file_path.BaseName();
  new_file_path_ = public_directory_.Append(base_name);

  // Update the file location in the OfflinePageModel database, then call the
  // callback.
  store_->Execute(
      base::BindOnce(&ChangeFileLocationSync, page_, new_file_path_),
      base::BindOnce(&MovePageTask::OnMovePageDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void MovePageTask::OnMovePageDone(bool result) {
  // SQL errors are ignored because there isn't much that the calling code can
  // do to fix the situation if we can't write to the DB.
  TaskComplete();

  // Call the callback here if the operation succeeded.
  if (result)
    std::move(callback_).Run(page_, new_file_path_);
}

}  // namespace offline_pages
