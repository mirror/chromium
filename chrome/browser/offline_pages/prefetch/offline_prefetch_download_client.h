// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_OFFLINE_PREFETCH_DOWNLOAD_CLIENT_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_OFFLINE_PREFETCH_DOWNLOAD_CLIENT_H_

#include "base/macros.h"
#include "components/download/public/client.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"

namespace content {
class BrowserContext;
}

namespace offline_pages {

class OfflinePrefetchDownloadClient : public download::Client {
 public:
  explicit OfflinePrefetchDownloadClient(content::BrowserContext* context);
  ~OfflinePrefetchDownloadClient() override;

 private:
  // Overridden from Client:
  void OnServiceInitialized(
      const std::vector<std::string>& outstanding_download_guids) override;
  download::Client::ShouldDownload OnDownloadStarted(
      const std::string& guid,
      const std::vector<GURL>& url_chain,
      const scoped_refptr<const net::HttpResponseHeaders>& headers) override;
  void OnDownloadUpdated(const std::string& guid,
                         uint64_t bytes_downloaded) override;
  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason) override;
  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size) override;

  PrefetchService::DownloadDelegate* GetDownloadDelegate() const;

  content::BrowserContext* context_;

  DISALLOW_COPY_AND_ASSIGN(OfflinePrefetchDownloadClient);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_OFFLINE_PREFETCH_DOWNLOAD_CLIENT_H_
