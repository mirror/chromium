// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/import_cleanup_task.h"

#include "base/bind.h"
#include "base/callback.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

namespace {

bool CleanupImportSync(sql::Connection* db) {
  if (!db)
    return false;

  static const char kSql[] =
      "UPDATE prefetch_items"
      " SET state = ?, error_code = ?"
      " WHERE state = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));
  statement.BindInt(1, static_cast<int>(PrefetchItemErrorCode::IMPORT_ABORTED));
  statement.BindInt(2, static_cast<int>(PrefetchItemState::IMPORTING));

  return statement.Run();
}

}  // namespace

ImportCleanupTask::ImportCleanupTask(PrefetchStore* prefetch_store)
    : prefetch_store_(prefetch_store), weak_ptr_factory_(this) {}

ImportCleanupTask::~ImportCleanupTask() {}

void ImportCleanupTask::Run() {
  prefetch_store_->Execute(base::BindOnce(&CleanupImportSync),
                           base::BindOnce(&ImportCleanupTask::OnFinished,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void ImportCleanupTask::OnFinished(bool success) {
  TaskComplete();
}

}  // namespace offline_pages
