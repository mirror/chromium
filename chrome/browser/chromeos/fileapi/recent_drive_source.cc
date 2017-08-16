// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_drive_source.h"

#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_system_url.h"

using content::BrowserThread;

namespace chromeos {

RecentDriveSource::RecentDriveSource(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RecentDriveSource::~RecentDriveSource() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RecentDriveSource::GetRecentFiles(RecentContext context,
                                       GetRecentFilesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  drive::FileSystemInterface* file_system =
      drive::util::GetFileSystemByProfile(profile_);
  if (!file_system) {
    // |file_system| is nullptr if Drive is disabled.
    OnSearchMetadata(std::move(context), std::move(callback),
                     drive::FILE_ERROR_FAILED, nullptr);
    return;
  }

  file_system->SearchMetadata(
      "" /* query */, drive::SEARCH_METADATA_EXCLUDE_DIRECTORIES,
      kMaxFilesFromSingleSource, drive::MetadataSearchOrder::LAST_MODIFIED,
      base::Bind(
          &RecentDriveSource::OnSearchMetadata, weak_ptr_factory_.GetWeakPtr(),
          base::Passed(std::move(context)), base::Passed(std::move(callback))));
}

void RecentDriveSource::OnSearchMetadata(
    RecentContext context,
    GetRecentFilesCallback callback,
    drive::FileError error,
    std::unique_ptr<drive::MetadataSearchResultVector> results) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error != drive::FILE_ERROR_OK) {
    std::move(callback).Run({});
    return;
  }

  storage::ExternalMountPoints* mount_points =
      storage::ExternalMountPoints::GetSystemInstance();
  // TODO(nya): Introduce a common function to get the drive mount point name.
  std::string drive_mount_name =
      drive::util::GetDriveMountPointPath(profile_).BaseName().AsUTF8Unsafe();
  base::FilePath drive_grant_root_path = drive::util::GetDriveGrandRootPath();

  std::vector<storage::FileSystemURL> files;

  for (const auto& result : *results) {
    if (result.is_directory)
      continue;

    base::FilePath path;
    drive_grant_root_path.AppendRelativePath(result.path, &path);
    files.emplace_back(mount_points->CreateExternalFileSystemURL(
        context.origin(), drive_mount_name, path));
  }

  std::move(callback).Run(std::move(files));
}

}  // namespace chromeos
