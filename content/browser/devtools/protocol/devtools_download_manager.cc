// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/devtools_download_manager.h"

#include <iterator>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "content/browser/devtools/protocol/devtools_download_item.h"
#include "content/browser/devtools/protocol/devtools_download_manager_helper.h"
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_task_runner.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_url_parameters.h"

namespace {
const char kDevToolsDownloadManagerKeyName[] = "devtools_download_manager";

void DownloadItemDetach(
    std::unique_ptr<content::protocol::DevToolsDownloadItem> download_item) {
  DCHECK(content::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
}

}  // namespace

namespace content {
namespace protocol {

// static
void DevToolsDownloadManager::CreateForBrowserContext(
    BrowserContext* browser_context) {
  if (FromBrowserContext(browser_context))
    return;

  DevToolsDownloadManager* download_manager =
      new DevToolsDownloadManager(browser_context);
  browser_context->SetUserData(kDevToolsDownloadManagerKeyName,
                               base::WrapUnique(download_manager));
}

// static
DevToolsDownloadManager* DevToolsDownloadManager::FromBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<DevToolsDownloadManager*>(
      browser_context->GetUserData(kDevToolsDownloadManagerKeyName));
}

DevToolsDownloadManager::DevToolsDownloadManager(
    BrowserContext* browser_context)
    : browser_context_(browser_context), shutdown_needed_(true) {
  DCHECK(browser_context);
}

DevToolsDownloadManager::~DevToolsDownloadManager() {
  DCHECK(!shutdown_needed_);
}

void DevToolsDownloadManager::CloseDownload(DownloadItem* download) {
  if (!download)
    return;
  GetDownloadTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&DevToolsDownloadManager::CloseDownloadOnRunner,
                                base::Unretained(this), download->GetGuid()));
}

void DevToolsDownloadManager::CloseDownloadOnRunner(const std::string& guid) {
  downloads_by_guid_.erase(guid);
}

void DevToolsDownloadManager::Shutdown() {
  shutdown_needed_ = false;

  for (auto& it : downloads_by_guid_) {
    GetDownloadTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(DownloadItemDetach, base::Passed(&it.second)));
  }
  downloads_by_guid_.clear();
};

void DevToolsDownloadManager::GetAllDownloads(DownloadVector* downloads) {
  for (const auto& it : downloads_by_guid_) {
    downloads->push_back(it.second.get());
  }
}

void DevToolsDownloadManager::StartDownload(
    std::unique_ptr<DownloadCreateInfo> info,
    std::unique_ptr<DownloadManager::InputStream> stream,
    const DownloadUrlParameters::OnStartedCallback& on_started) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(info);

  DevToolsDownloadManagerHelper* download_helper =
      DevToolsDownloadManagerHelper::FromWebContents(
          info->request_handle->GetWebContents());
  DCHECK(download_helper);

  DevToolsDownloadItem* download = new DevToolsDownloadItem(*info);
  downloads_by_guid_[download->GetGuid()] = base::WrapUnique(download);
  GetDownloadTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&DevToolsDownloadItem::Initialize,
                                base::Unretained(download), std::move(stream),
                                std::move(info->request_handle)));
  // Notify download has started.
  if (!download_helper->GetCallback().is_null())
    download_helper->GetCallback().Run(download,
                                       DOWNLOAD_INTERRUPT_REASON_NONE);
}

int DevToolsDownloadManager::InProgressCount() const {
  int count = 0;
  for (const auto& it : downloads_by_guid_) {
    if (it.second->GetState() == DownloadItem::IN_PROGRESS)
      ++count;
  }
  return count;
}

int DevToolsDownloadManager::NonMaliciousInProgressCount() const {
  return InProgressCount();
}

BrowserContext* DevToolsDownloadManager::GetBrowserContext() const {
  return browser_context_;
}

DownloadItem* DevToolsDownloadManager::GetDownloadByGuid(
    const std::string& guid) {
  return base::ContainsKey(downloads_by_guid_, guid)
             ? downloads_by_guid_[guid].get()
             : nullptr;
}

DownloadManagerDelegate* DevToolsDownloadManager::GetDelegate() const {
  return nullptr;
}

DownloadItem* DevToolsDownloadManager::CreateDownloadItem(
    const std::string& guid,
    uint32_t id,
    const base::FilePath& current_path,
    const base::FilePath& target_path,
    const std::vector<GURL>& url_chain,
    const GURL& referrer_url,
    const GURL& site_url,
    const GURL& tab_url,
    const GURL& tab_refererr_url,
    const std::string& mime_type,
    const std::string& original_mime_type,
    base::Time start_time,
    base::Time end_time,
    const std::string& etag,
    const std::string& last_modified,
    int64_t received_bytes,
    int64_t total_bytes,
    const std::string& hash,
    DownloadItem::DownloadState state,
    DownloadDangerType danger_type,
    DownloadInterruptReason interrupt_reason,
    bool opened,
    base::Time last_access_time,
    bool transient,
    const std::vector<DownloadItem::ReceivedSlice>& received_slices) {
  return nullptr;
}

bool DevToolsDownloadManager::IsManagerInitialized() const {
  return true;
};

int DevToolsDownloadManager::RemoveDownloadsByURLAndTime(
    const base::Callback<bool(const GURL&)>& url_filter,
    base::Time remove_begin,
    base::Time remove_end) {
  return 0;
}

DownloadItem* DevToolsDownloadManager::GetDownload(uint32_t download_id) {
  return nullptr;
}

}  // namespace protocol
}  // namespace content
