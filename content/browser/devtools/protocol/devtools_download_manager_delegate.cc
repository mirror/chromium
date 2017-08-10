// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/devtools_download_manager_delegate.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/devtools/protocol/devtools_download_manager_helper.h"
#include "content/public/browser/browser_context.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"

namespace content {

class WebContents;

namespace protocol {

DevToolsDownloadManagerDelegate::DevToolsDownloadManagerDelegate()
    : download_manager_(nullptr), weak_ptr_factory_(this) {}

DevToolsDownloadManagerDelegate::~DevToolsDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<content::DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(nullptr);
    download_manager_ = nullptr;
  }
}

void DevToolsDownloadManagerDelegate::SetDownloadManager(
    content::DownloadManager* download_manager) {
  download_manager_ = download_manager;
}

void DevToolsDownloadManagerDelegate::Shutdown() {
  // Revoke any pending callbacks. download_manager_ et. al. are no longer safe
  // to access after this point.
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool DevToolsDownloadManagerDelegate::DetermineDownloadTarget(
    content::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DevToolsDownloadManagerHelper* download_helper =
      DevToolsDownloadManagerHelper::FromWebContents(
          download->GetWebContents());

  if (!download_helper ||
      download_helper->GetDownloadBehavior() ==
          DevToolsDownloadManagerHelper::DownloadBehavior::DENY) {
    base::FilePath empty_path = base::FilePath();
    callback.Run(empty_path,
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, empty_path,
                 content::DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED);
    return true;
  }

  base::FilePath download_path =
      base::FilePath::FromUTF8Unsafe(download_helper->GetDownloadPath());
  // This assignment needs to be here because even at the call to
  // SetDownloadManager, the system is not fully initialized.
  if (download_path.empty()) {
    download_path = download_manager_->GetBrowserContext()->GetPath().Append(
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
      base::Bind(&DevToolsDownloadManagerDelegate::OnDownloadPathGenerated,
                 weak_ptr_factory_.GetWeakPtr(), download->GetId(), callback);

  PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
       base::TaskPriority::USER_VISIBLE},
      base::Bind(&DevToolsDownloadManagerDelegate::GenerateFilename,
                 download->GetURL(), download->GetContentDisposition(),
                 download->GetSuggestedFilename(), download->GetMimeType(),
                 download_path, filename_determined_callback));
  return true;
}

bool DevToolsDownloadManagerDelegate::ShouldOpenDownload(
    content::DownloadItem* item,
    const content::DownloadOpenDelayedCallback& callback) {
  return true;
}

void DevToolsDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

// static
void DevToolsDownloadManagerDelegate::GenerateFilename(
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

void DevToolsDownloadManagerDelegate::OnDownloadPathGenerated(
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

}  // namespace protocol
}  // namespace content
