// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_download_manager_delegate.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"

namespace headless {

HeadlessDownloadManagerDelegate::HeadlessDownloadManagerDelegate()
    : download_manager_(nullptr),
      download_behavior_(DownloadBehavior::DENY),
      weak_ptr_factory_(this) {}

HeadlessDownloadManagerDelegate::~HeadlessDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<content::DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(nullptr);
    download_manager_ = nullptr;
  }
}

void HeadlessDownloadManagerDelegate::SetDownloadManager(
    content::DownloadManager* download_manager) {
  download_manager_ = download_manager;
}

void HeadlessDownloadManagerDelegate::Shutdown() {
  // Revoke any pending callbacks. download_manager_ et. al. are no longer safe
  // to access after this point.
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool HeadlessDownloadManagerDelegate::DetermineDownloadTarget(
    content::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (download_behavior_ == DownloadBehavior::DENY) {
    base::FilePath empty_path = base::FilePath();
    callback.Run(empty_path,
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, empty_path,
                 content::DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED);
    return true;
  }

  // This assignment needs to be here because even at the call to
  // SetDownloadManager, the system is not fully initialized.
  if (default_download_path_.empty()) {
    default_download_path_ =
        download_manager_->GetBrowserContext()->GetPath().Append(
            FILE_PATH_LITERAL("Downloads"));
  }

  if (!download->GetForcedFilePath().empty()) {
    callback.Run(download->GetForcedFilePath(),
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetForcedFilePath(),
                 content::DOWNLOAD_INTERRUPT_REASON_NONE);
    return true;
  }

  FilenameDeterminedCallback filename_determined_callback =
      base::Bind(&HeadlessDownloadManagerDelegate::OnDownloadPathGenerated,
                 weak_ptr_factory_.GetWeakPtr(), download->GetId(), callback);

  PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
       base::TaskPriority::USER_VISIBLE},
      base::Bind(&HeadlessDownloadManagerDelegate::GenerateFilename,
                 download->GetURL(), download->GetContentDisposition(),
                 download->GetSuggestedFilename(), download->GetMimeType(),
                 default_download_path_, filename_determined_callback));
  return true;
}

bool HeadlessDownloadManagerDelegate::ShouldOpenDownload(
    content::DownloadItem* item,
    const content::DownloadOpenDelayedCallback& callback) {
  return true;
}

void HeadlessDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

// static
void HeadlessDownloadManagerDelegate::GenerateFilename(
    const GURL& url,
    const std::string& content_disposition,
    const std::string& suggested_filename,
    const std::string& mime_type,
    const base::FilePath& suggested_directory,
    const FilenameDeterminedCallback& callback) {
  base::ThreadRestrictions::AssertIOAllowed();
  base::FilePath generated_name =
      net::GenerateFileName(url, content_disposition, std::string(),
                            suggested_filename, mime_type, "download");

  if (!base::PathExists(suggested_directory))
    base::CreateDirectory(suggested_directory);

  base::FilePath suggested_path(suggested_directory.Append(generated_name));
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::Bind(callback, suggested_path));
}

void HeadlessDownloadManagerDelegate::OnDownloadPathGenerated(
    uint32_t download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& suggested_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  callback.Run(suggested_path,
               content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
               suggested_path.AddExtension(FILE_PATH_LITERAL(".crdownload")),
               content::DOWNLOAD_INTERRUPT_REASON_NONE);
}

void HeadlessDownloadManagerDelegate::SetDownloadBehavior(
    DownloadBehavior behavior) {
  download_behavior_ = behavior;
}

void HeadlessDownloadManagerDelegate::SetDownloadDirectory(
    const base::FilePath& path) {
  default_download_path_ = path;
}

}  // namespace headless
