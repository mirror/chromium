// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/web_database_host_impl.h"

#include <string>
#include <utility>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "storage/browser/database/database_util.h"
#include "storage/browser/database/vfs_backend.h"
#include "third_party/sqlite/sqlite3.h"

using storage::DatabaseUtil;
using storage::VfsBackend;

namespace content {

WebDatabaseHostImpl::WebDatabaseHostImpl(
    scoped_refptr<storage::DatabaseTracker> db_tracker)
    : db_tracker_(std::move(db_tracker)) {
  DCHECK(db_tracker_.get());
}

WebDatabaseHostImpl::~WebDatabaseHostImpl() = default;

void WebDatabaseHostImpl::Create(
    scoped_refptr<storage::DatabaseTracker> db_tracker,
    content::mojom::WebDatabaseHostRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<WebDatabaseHostImpl>(std::move(db_tracker)),
      std::move(request));
}

void WebDatabaseHostImpl::OpenFile(const base::string16& vfs_file_name,
                                   int32_t desired_flags,
                                   OpenFileCallback callback) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  base::File file;
  const base::File* tracked_file = nullptr;
  std::string origin_identifier;
  base::string16 database_name;

  // When in incognito mode, we want to make sure that all DB files are
  // removed when the incognito browser context goes away, so we add the
  // SQLITE_OPEN_DELETEONCLOSE flag when opening all files, and keep
  // open handles to them in the database tracker to make sure they're
  // around for as long as needed.
  if (vfs_file_name.empty()) {
    file = VfsBackend::OpenTempFileInDirectory(db_tracker_->DatabaseDirectory(),
                                               desired_flags);
  } else if (DatabaseUtil::CrackVfsFileName(vfs_file_name, &origin_identifier,
                                            &database_name, nullptr) &&
             !db_tracker_->IsDatabaseScheduledForDeletion(origin_identifier,
                                                          database_name)) {
    base::FilePath db_file = DatabaseUtil::GetFullFilePathForVfsFile(
        db_tracker_.get(), vfs_file_name);
    if (!db_file.empty()) {
      if (db_tracker_->IsIncognitoProfile()) {
        tracked_file = db_tracker_->GetIncognitoFile(vfs_file_name);
        if (!tracked_file) {
          file = VfsBackend::OpenFile(
              db_file, desired_flags | SQLITE_OPEN_DELETEONCLOSE);
          if (!(desired_flags & SQLITE_OPEN_DELETEONCLOSE)) {
            tracked_file =
                db_tracker_->SaveIncognitoFile(vfs_file_name, std::move(file));
          }
        }
      } else {
        file = VfsBackend::OpenFile(db_file, desired_flags);
      }
    }
  }

  base::File result;
  if (file.IsValid()) {
    result = std::move(file);
  } else if (tracked_file) {
    DCHECK(tracked_file->IsValid());
    result = base::File(tracked_file->GetPlatformFile());
  }
  std::move(callback).Run(std::move(result));
}

}  // namespace content
