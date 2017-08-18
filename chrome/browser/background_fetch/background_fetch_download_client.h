// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DOWNLOAD_CLIENT_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DOWNLOAD_CLIENT_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/download/public/client.h"

namespace download {
class DownloadService;
}

namespace content {
class BackgroundFetchDelegate;
class BrowserContext;
}  // namespace content

namespace background_fetch {

class BackgroundFetchDownloadClient : public download::Client {
 public:
  explicit BackgroundFetchDownloadClient(content::BrowserContext* context);
  ~BackgroundFetchDownloadClient() override;

 private:
  // Overridden from Client:
  void OnServiceInitialized(
      bool state_lost,
      const std::vector<std::string>& outstanding_download_guids) override;
  void OnServiceUnavailable() override;
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

  content::BrowserContext* browser_context_;
  content::BackgroundFetchDelegate* delegate_;
  download::DownloadService* download_service_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDownloadClient);
};

}  // namespace background_fetch

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DOWNLOAD_CLIENT_H_
