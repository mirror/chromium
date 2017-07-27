// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_arc_media_source.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_content_file_system_file_stream_reader.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_content_file_system_url_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner_util.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "components/arc/arc_service_manager.h"
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

class ArcMediaRecentFile : public RecentFile {
 public:
  explicit ArcMediaRecentFile(arc::mojom::DocumentPtr document);
  ~ArcMediaRecentFile() override;

  void GetFileInfo(int fields,
                   storage::FileSystemContext* file_system_context,
                   const GetFileInfoCallback& callback) override;

  std::unique_ptr<storage::FileStreamReader> CreateFileStreamReader(
      int64_t offset,
      int64_t max_bytes_to_read,
      const base::Time& expected_modification_time,
      storage::FileSystemContext* file_system_context) override;

 private:
  static std::string BuildUniqueId(const arc::mojom::DocumentPtr& document);

  const arc::mojom::DocumentPtr document_;

  DISALLOW_COPY_AND_ASSIGN(ArcMediaRecentFile);
};

ArcMediaRecentFile::ArcMediaRecentFile(arc::mojom::DocumentPtr document)
    : RecentFile(RecentFile::SourceType::ARC_MEDIA,
                 arc::GetFileNameForDocument(document),
                 document->document_id),
      document_(std::move(document)) {}

ArcMediaRecentFile::~ArcMediaRecentFile() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void ArcMediaRecentFile::GetFileInfo(
    int fields,
    storage::FileSystemContext* file_system_context,
    const GetFileInfoCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::File::Info info;
  info.size = document_->size;
  info.is_directory = document_->mime_type == arc::kAndroidDirectoryMimeType;
  info.is_symbolic_link = false;
  info.last_modified = info.last_accessed = info.creation_time =
      base::Time::FromJavaTime(document_->last_modified);
  callback.Run(base::File::FILE_OK, info);
}

std::unique_ptr<storage::FileStreamReader>
ArcMediaRecentFile::CreateFileStreamReader(
    int64_t offset,
    int64_t max_bytes_to_read,
    const base::Time& expected_modification_time,
    storage::FileSystemContext* file_system_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  GURL content_url = arc::BuildDocumentUrl(kMediaDocumentsProviderAuthority,
                                           document_->document_id);
  return base::MakeUnique<arc::ArcContentFileSystemFileStreamReader>(
      content_url, offset);
}

}  // namespace

class RecentArcMediaSource::RecentFileLister {
 public:
  RecentFileLister();
  ~RecentFileLister();

  void Run(RecentContext context, GetRecentFilesCallback callback);

 private:
  void OnGetRecentDocuments(
      const std::string& root_id,
      base::Optional<std::vector<arc::mojom::DocumentPtr>> maybe_documents);

  RecentContext context_;
  GetRecentFilesCallback callback_;

  int num_inflight_roots_;
  std::vector<std::unique_ptr<RecentFile>> files_;

  base::WeakPtrFactory<RecentFileLister> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecentFileLister);
};

RecentArcMediaSource::RecentFileLister::RecentFileLister()
    : num_inflight_roots_(-1), weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

RecentArcMediaSource::RecentFileLister::~RecentFileLister() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void RecentArcMediaSource::RecentFileLister::Run(
    RecentContext context,
    GetRecentFilesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  context_ = std::move(context);
  callback_ = std::move(callback);

  num_inflight_roots_ = arraysize(kMediaDocumentsProviderRootIds);

  for (const char* root_id : kMediaDocumentsProviderRootIds) {
    arc::file_system_operation_runner_util::GetRecentDocumentsOnIOThread(
        kMediaDocumentsProviderAuthority, root_id,
        base::Bind(&RecentFileLister::OnGetRecentDocuments,
                   weak_ptr_factory_.GetWeakPtr(), std::string(root_id)));
  }
}

void RecentArcMediaSource::RecentFileLister::OnGetRecentDocuments(
    const std::string& root_id,
    base::Optional<std::vector<arc::mojom::DocumentPtr>> maybe_documents) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (maybe_documents.has_value()) {
    for (auto& document : maybe_documents.value()) {
      if (document->android_file_system_path.has_value() &&
          base::StartsWith(document->android_file_system_path.value(),
                           kAndroidDownloadDirPrefix,
                           base::CompareCase::SENSITIVE))
        continue;

      files_.emplace_back(
          base::MakeUnique<ArcMediaRecentFile>(std::move(document)));
    }
  }

  --num_inflight_roots_;
  if (num_inflight_roots_ == 0)
    std::move(callback_).Run(std::move(files_));
}

RecentArcMediaSource::RecentArcMediaSource() : weak_ptr_factory_(this) {}

RecentArcMediaSource::~RecentArcMediaSource() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void RecentArcMediaSource::GetRecentFiles(RecentContext context,
                                          GetRecentFilesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&arc::IsArcAllowedForProfile, context.profile()),
      base::BindOnce(&RecentArcMediaSource::OnIsArcAllowedForProfile,
                     weak_ptr_factory_.GetWeakPtr(), context,
                     base::Passed(std::move(callback))));
}

void RecentArcMediaSource::OnIsArcAllowedForProfile(
    RecentContext context,
    GetRecentFilesCallback callback,
    bool is_arc_allowed) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!is_arc_allowed) {
    std::move(callback).Run({});
    return;
  }

  std::unique_ptr<RecentFileLister> lister =
      base::MakeUnique<RecentFileLister>();
  RecentFileLister* lister_ptr = lister.get();
  lister_ptr->Run(std::move(context),
                  base::BindOnce(&RecentArcMediaSource::OnRecentFileListed,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 base::Passed(std::move(lister)),
                                 base::Passed(std::move(callback))));
}

void RecentArcMediaSource::OnRecentFileListed(
    std::unique_ptr<RecentFileLister> lister,
    GetRecentFilesCallback callback,
    std::vector<std::unique_ptr<RecentFile>> files) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(std::move(files));
}

}  // namespace chromeos
