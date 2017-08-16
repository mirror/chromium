// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_arc_media_source.h"

#include <iterator>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/common/file_system.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/external_mount_points.h"

using content::BrowserThread;

namespace chromeos {

namespace {

const char kAndroidDownloadDirPrefix[] = "/storage/emulated/0/Download/";

const char kMediaDocumentsProviderAuthority[] =
    "com.android.providers.media.documents";
const char* kMediaDocumentsProviderRootIds[] = {
    "images_root", "videos_root", "audio_root",
};

base::FilePath GetRelativeMountPath(const std::string& root_id) {
  base::FilePath mount_path = arc::GetDocumentsProviderMountPath(
      kMediaDocumentsProviderAuthority,
      // In MediaDocumentsProvider, |root_id| and |root_document_id| are
      // the same.
      root_id);
  base::FilePath relative_mount_path;
  base::FilePath(arc::kDocumentsProviderMountPointPath)
      .AppendRelativePath(mount_path, &relative_mount_path);
  return relative_mount_path;
}

}  // namespace

class RecentArcMediaSource::MediaRoot {
 public:
  MediaRoot(const std::string& root_id, Profile* profile);
  ~MediaRoot();

  void GetRecentFiles(RecentContext context, GetRecentFilesCallback callback);

 private:
  void OnGetRecentDocuments(
      base::Optional<std::vector<arc::mojom::DocumentPtr>> maybe_documents);
  void ScanDirectory(const base::FilePath& path);
  void OnReadDirectoryExtra(
      const base::FilePath& path,
      base::File::Error result,
      std::vector<arc::ArcDocumentsProviderRoot::FileInfo> files);
  void OnComplete();

  storage::FileSystemURL BuildDocumentsProviderUrl(
      const base::FilePath& path) const;

  const std::string root_id_;
  Profile* const profile_;
  const base::FilePath relative_mount_path_;

  RecentContext context_;
  GetRecentFilesCallback callback_;

  int num_inflight_readdirs_ = 0;
  std::set<std::string> unknown_document_ids_;

  RecentFileList files_;

  base::WeakPtrFactory<MediaRoot> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaRoot);
};

RecentArcMediaSource::MediaRoot::MediaRoot(const std::string& root_id,
                                           Profile* profile)
    : root_id_(root_id),
      profile_(profile),
      relative_mount_path_(GetRelativeMountPath(root_id)),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RecentArcMediaSource::MediaRoot::~MediaRoot() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RecentArcMediaSource::MediaRoot::GetRecentFiles(
    RecentContext context,
    GetRecentFilesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!context_.is_valid());
  DCHECK(callback_.is_null());
  DCHECK_EQ(0, num_inflight_readdirs_);
  DCHECK(unknown_document_ids_.empty());

  context_ = std::move(context);
  callback_ = std::move(callback);

  auto* runner =
      arc::ArcFileSystemOperationRunner::GetForBrowserContext(profile_);
  if (!runner) {
    OnComplete();
    return;
  }

  runner->GetRecentDocuments(kMediaDocumentsProviderAuthority, root_id_,
                             base::Bind(&MediaRoot::OnGetRecentDocuments,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void RecentArcMediaSource::MediaRoot::OnGetRecentDocuments(
    base::Optional<std::vector<arc::mojom::DocumentPtr>> maybe_documents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());
  DCHECK_EQ(0, num_inflight_readdirs_);
  DCHECK(unknown_document_ids_.empty());

  if (maybe_documents.has_value()) {
    for (auto& document : maybe_documents.value()) {
      if (document->android_file_system_path.has_value() &&
          base::StartsWith(document->android_file_system_path.value(),
                           kAndroidDownloadDirPrefix,
                           base::CompareCase::SENSITIVE))
        continue;

      unknown_document_ids_.insert(document->document_id);
    }
  }

  if (unknown_document_ids_.empty()) {
    OnComplete();
    return;
  }

  ScanDirectory(base::FilePath());
}

void RecentArcMediaSource::MediaRoot::ScanDirectory(
    const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());

  auto* root_map =
      arc::ArcDocumentsProviderRootMap::GetForBrowserContext(profile_);
  if (!root_map) {
    OnReadDirectoryExtra(path, base::File::FILE_ERROR_FAILED, {});
    return;
  }

  // In MediaDocumentsProvider, |root_id| and |root_document_id| are the same.
  auto* root = root_map->Lookup(kMediaDocumentsProviderAuthority, root_id_);
  if (!root) {
    OnReadDirectoryExtra(path, base::File::FILE_ERROR_NOT_FOUND, {});
    return;
  }

  ++num_inflight_readdirs_;
  root->ReadDirectoryExtra(
      path, base::Bind(&RecentArcMediaSource::MediaRoot::OnReadDirectoryExtra,
                       weak_ptr_factory_.GetWeakPtr(), path));
}

void RecentArcMediaSource::MediaRoot::OnReadDirectoryExtra(
    const base::FilePath& path,
    base::File::Error result,
    std::vector<arc::ArcDocumentsProviderRoot::FileInfo> files) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());

  for (const auto& file : files) {
    base::FilePath subpath = path.Append(file.name);
    if (file.is_directory) {
      ScanDirectory(subpath);
      continue;
    }

    if (unknown_document_ids_.count(file.document_id) > 0) {
      files_.emplace_back(BuildDocumentsProviderUrl(subpath));
      unknown_document_ids_.erase(file.document_id);
    }
  }

  --num_inflight_readdirs_;

  if (num_inflight_readdirs_ == 0)
    OnComplete();
}

void RecentArcMediaSource::MediaRoot::OnComplete() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());
  DCHECK_EQ(0, num_inflight_readdirs_);

  unknown_document_ids_.clear();

  context_ = RecentContext();
  GetRecentFilesCallback callback;
  std::swap(callback, callback_);
  RecentFileList files;
  std::swap(files, files_);
  std::move(callback).Run(std::move(files));
}

storage::FileSystemURL
RecentArcMediaSource::MediaRoot::BuildDocumentsProviderUrl(
    const base::FilePath& path) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());

  storage::ExternalMountPoints* mount_points =
      storage::ExternalMountPoints::GetSystemInstance();

  return mount_points->CreateExternalFileSystemURL(
      context_.origin(), arc::kDocumentsProviderMountPointName,
      relative_mount_path_.Append(path));
}

RecentArcMediaSource::RecentArcMediaSource(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (const char* root_id : kMediaDocumentsProviderRootIds)
    roots_.emplace_back(base::MakeUnique<MediaRoot>(root_id, profile_));
}

RecentArcMediaSource::~RecentArcMediaSource() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RecentArcMediaSource::GetRecentFiles(RecentContext context,
                                          GetRecentFilesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!context_.is_valid());
  DCHECK(callback_.is_null());
  DCHECK_EQ(0, num_inflight_roots_);
  DCHECK(files_.empty());

  context_ = std::move(context);
  callback_ = std::move(callback);

  if (!arc::IsArcPlayStoreEnabledForProfile(profile_)) {
    OnComplete();
    return;
  }

  num_inflight_roots_ = roots_.size();
  if (num_inflight_roots_ == 0) {
    OnComplete();
    return;
  }

  for (auto& root : roots_) {
    root->GetRecentFiles(
        context_, base::BindOnce(&RecentArcMediaSource::OnGetRecentFilesForRoot,
                                 weak_ptr_factory_.GetWeakPtr()));
  }
}

void RecentArcMediaSource::OnGetRecentFilesForRoot(RecentFileList files) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());

  files_.insert(files_.end(), std::make_move_iterator(files.begin()),
                std::make_move_iterator(files.end()));

  --num_inflight_roots_;
  if (num_inflight_roots_ == 0)
    OnComplete();
}

void RecentArcMediaSource::OnComplete() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context_.is_valid());
  DCHECK(!callback_.is_null());
  DCHECK_EQ(0, num_inflight_roots_);

  context_ = RecentContext();
  GetRecentFilesCallback callback;
  std::swap(callback, callback_);
  RecentFileList files;
  std::swap(files, files_);
  std::move(callback).Run(std::move(files));
}

}  // namespace chromeos
