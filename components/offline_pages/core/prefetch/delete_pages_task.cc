// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/delete_pages_task.h"

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {
namespace {
// A Task that deletes a page from the prefetch pipeline that matches a
// ClientId, but only when the page has not been (at least partially)
// downloaded.
class DeleteWithClientIdIfNotDownloaded : public Task {
 public:
  // Result of executing the command in the store.
  enum class Result {
    PAGES_REMOVED,
    STORE_ERROR,
  };

  DeleteWithClientIdIfNotDownloaded(PrefetchStore* prefetch_store,
                                    const ClientId& client_id)
      : prefetch_store_(prefetch_store),
        client_id_(client_id),
        weak_ptr_factory_(this) {
    DCHECK(prefetch_store_);
  }
  ~DeleteWithClientIdIfNotDownloaded() override {}

  void Run() override {
    prefetch_store_->Execute<Result>(
        base::BindOnce(&DeletePageByClientIdIfNotDownloadedSync, client_id_),
        base::BindOnce(&DeleteWithClientIdIfNotDownloaded::OnComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  static Result DeletePageByClientIdIfNotDownloadedSync(
      const ClientId& client_id,
      sql::Connection* db) {
    if (!db)
      return Result::STORE_ERROR;
    // If file_path='', the download hasn't been started.
    static const char kSql[] =
        "DELETE FROM prefetch_items WHERE client_id=? AND client_namespace=?"
        " AND file_path=''";

    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindString(0, client_id.id);
    statement.BindString(1, client_id.name_space);

    return statement.Run() ? Result::PAGES_REMOVED : Result::STORE_ERROR;
  }
  void OnComplete(Result result) { TaskComplete(); }

  PrefetchStore* prefetch_store_;
  ClientId client_id_;
  base::WeakPtrFactory<DeleteWithClientIdIfNotDownloaded> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DeleteWithClientIdIfNotDownloaded);
};
}  // namespace

std::unique_ptr<Task> DeletePagesTask::WithClientIdIfNotDownloaded(
    PrefetchStore* prefetch_store,
    const ClientId& client_id) {
  return std::unique_ptr<Task>(
      new DeleteWithClientIdIfNotDownloaded(prefetch_store, client_id));
}

}  // namespace offline_pages
