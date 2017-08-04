// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_download_source.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/file_manager/path_util.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"
#include "storage/browser/fileapi/file_system_url.h"

using content::BrowserThread;

namespace chromeos {

struct RecentDownloadSource::FileSystemURLWithLastModified {
  storage::FileSystemURL url;
  base::Time last_modified;

  friend bool operator<(const FileSystemURLWithLastModified& a,
                        const FileSystemURLWithLastModified& b) {
    return a.last_modified < b.last_modified;
  }
};

RecentDownloadSource::RecentDownloadSource(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {}

RecentDownloadSource::~RecentDownloadSource() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void RecentDownloadSource::GetRecentFiles(RecentContext context,
                                          GetRecentFilesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(!context_.is_valid());
  DCHECK(callback_.is_null());
  DCHECK_EQ(0, inflight_reads_);
  DCHECK_EQ(0, inflight_stats_);
  DCHECK(top_entries_.empty());

  context_ = std::move(context);
  callback_ = std::move(callback);

  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());

  if (mount_point_name_.has_value()) {
    ScanDirectory(base::FilePath());
    return;
  }

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&file_manager::util::GetDownloadsMountPointName, profile_),
      base::BindOnce(&RecentDownloadSource::OnGetDownloadsMountPointName,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RecentDownloadSource::OnGetDownloadsMountPointName(
    const std::string& mount_point_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  mount_point_name_ = mount_point_name;

  DCHECK_EQ(0, inflight_reads_);
  DCHECK_EQ(0, inflight_stats_);
  DCHECK(top_entries_.empty());
  ScanDirectory(base::FilePath());
}

void RecentDownloadSource::ScanDirectory(const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(context_.is_valid());
  DCHECK(mount_point_name_.has_value());

  storage::ExternalMountPoints* mount_points =
      storage::ExternalMountPoints::GetSystemInstance();
  storage::FileSystemURL url = mount_points->CreateExternalFileSystemURL(
      context_.origin(), mount_point_name_.value(), path);

  ++inflight_reads_;
  context_.file_system_context()->operation_runner()->ReadDirectory(
      url, base::Bind(&RecentDownloadSource::OnReadDirectory,
                      weak_ptr_factory_.GetWeakPtr(), path));
}

void RecentDownloadSource::OnReadDirectory(
    const base::FilePath& path,
    base::File::Error result,
    const storage::FileSystemOperation::FileEntryList& entries,
    bool has_more) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(context_.is_valid());
  DCHECK(mount_point_name_.has_value());

  storage::ExternalMountPoints* mount_points =
      storage::ExternalMountPoints::GetSystemInstance();

  for (const auto& entry : entries) {
    base::FilePath subpath = path.Append(entry.name);
    if (entry.is_directory) {
      ScanDirectory(subpath);
    } else {
      storage::FileSystemURL url = mount_points->CreateExternalFileSystemURL(
          context_.origin(), mount_point_name_.value(), subpath);
      ++inflight_stats_;
      context_.file_system_context()->operation_runner()->GetMetadata(
          url, storage::FileSystemOperation::GET_METADATA_FIELD_LAST_MODIFIED,
          base::Bind(&RecentDownloadSource::OnGetMetadata,
                     weak_ptr_factory_.GetWeakPtr(), url));
    }
  }

  if (has_more)
    return;

  --inflight_reads_;
  OnReadOrStatFinished();
}

void RecentDownloadSource::OnGetMetadata(const storage::FileSystemURL& url,
                                         base::File::Error result,
                                         const base::File::Info& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (result == base::File::FILE_OK) {
    top_entries_.emplace(
        FileSystemURLWithLastModified{url, info.last_modified});
    while (top_entries_.size() > kMaxFilesFromSingleSource)
      top_entries_.pop();
  }

  --inflight_stats_;
  OnReadOrStatFinished();
}

void RecentDownloadSource::OnReadOrStatFinished() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (inflight_reads_ > 0 || inflight_stats_ > 0)
    return;

  // All reads/scans completed.
  RecentFileList files;
  while (!top_entries_.empty()) {
    files.emplace_back(top_entries_.top().url);
    top_entries_.pop();
  }

  context_ = RecentContext();
  GetRecentFilesCallback callback = std::move(callback_);

  DCHECK(!context_.is_valid());
  DCHECK(callback_.is_null());
  DCHECK_EQ(0, inflight_reads_);
  DCHECK_EQ(0, inflight_stats_);
  DCHECK(top_entries_.empty());

  std::move(callback).Run(std::move(files));
}

}  // namespace chromeos
