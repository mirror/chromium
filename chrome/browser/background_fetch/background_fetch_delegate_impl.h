// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_

#include "content/public/browser/background_fetch_delegate.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/download/public/download_params.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace download {
class DownloadService;
}  // namespace download

class BackgroundFetchDelegateImpl : public content::BackgroundFetchDelegate,
                                    public KeyedService {
 public:
  explicit BackgroundFetchDelegateImpl(
      content::BrowserContext* browser_context);

  ~BackgroundFetchDelegateImpl() override;

  void Shutdown() override;

  void DownloadUrl(const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override;

  void OnDownloadStarted(const std::string& guid,
                         std::unique_ptr<content::BackgroundFetchResponse>);

  void OnDownloadUpdated(const std::string& guid, uint64_t bytes_downloaded);

  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason);

  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size);

 private:
  void OnDownloadReceived(const std::string& guid,
                          download::DownloadParams::StartResult result);

  download::DownloadService* download_service_;

  base::WeakPtrFactory<BackgroundFetchDelegateImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateImpl);
};

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
