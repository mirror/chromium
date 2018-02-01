// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "content/browser/download/url_download_handler.h"
#include "content/browser/loader/navigation_url_loader.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/ssl_status.h"

namespace content {
class BrowserContext;
class DownloadItem;
class DownloadUrlParameters;

namespace protocol {

class DevToolsDownloadItem;

class CONTENT_EXPORT DevToolsDownloadManager : public DownloadManager {
 public:
  static void CreateForBrowserContext(BrowserContext* browser_context);
  static DevToolsDownloadManager* FromBrowserContext(
      BrowserContext* browser_context);

  DevToolsDownloadManager(BrowserContext* browser_context);
  ~DevToolsDownloadManager() override;

  void CloseDownload(DownloadItem* download);

  // Implemented DownloadManager functions.
  void GetAllDownloads(DownloadVector* result) override;
  void StartDownload(
      std::unique_ptr<DownloadCreateInfo> info,
      std::unique_ptr<DownloadManager::InputStream> stream,
      const DownloadUrlParameters::OnStartedCallback& on_started) override;
  void Shutdown() override;

  int InProgressCount() const override;
  int NonMaliciousInProgressCount() const override;
  BrowserContext* GetBrowserContext() const override;
  DownloadItem* GetDownloadByGuid(const std::string& guid) override;

  // Not Implemented DownloadManager functions.
  void SetDelegate(DownloadManagerDelegate* delegate) override{};
  DownloadManagerDelegate* GetDelegate() const override;
  int RemoveDownloadsByURLAndTime(
      const base::Callback<bool(const GURL&)>& url_filter,
      base::Time remove_begin,
      base::Time remove_end) override;
  void DownloadUrl(std::unique_ptr<DownloadUrlParameters> params) override{};
  void AddObserver(Observer* observer) override{};
  void RemoveObserver(Observer* observer) override{};
  content::DownloadItem* CreateDownloadItem(
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
      content::DownloadItem::DownloadState state,
      DownloadDangerType danger_type,
      DownloadInterruptReason interrupt_reason,
      bool opened,
      base::Time last_access_time,
      bool transient,
      const std::vector<DownloadItem::ReceivedSlice>& received_slices) override;
  void PostInitialization(
      DownloadInitializationDependency dependency) override{};
  bool IsManagerInitialized() const override;
  void CheckForHistoryFilesRemoval() override{};
  DownloadItem* GetDownload(uint32_t id) override;

 private:
  void CloseDownloadOnRunner(const std::string& guid);

  using DownloadGuidMap =
      std::unordered_map<std::string, std::unique_ptr<DevToolsDownloadItem>>;

  // Maps GUID to download item. All downloads are owned by the manager.
  DownloadGuidMap downloads_by_guid_;

  // The current active browser context.
  BrowserContext* browser_context_;

  // True if the download manager has been initialized and requires a shutdown.
  bool shutdown_needed_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDownloadManager);
};

}  // namespace protocol

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_H_
