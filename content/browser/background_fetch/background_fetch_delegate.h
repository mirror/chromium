// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

class GURL;

namespace net {
class HttpRequestHeaders;
struct NetworkTrafficAnnotationTag;
}  // namespace net

namespace content {
struct BackgroundFetchResponse;
struct BackgroundFetchResult;

// Must only be used on the UI thread.
class BackgroundFetchDelegate {
 public:
  class Client {
   public:
    virtual void OnDownloadUpdated(const std::string& guid,
                                   uint64_t bytes_downloaded) = 0;

    virtual void OnDownloadComplete(
        const std::string& guid,
        std::unique_ptr<BackgroundFetchResult> result) = 0;

    virtual void OnDownloadStarted(
        const std::string& guid,
        std::unique_ptr<content::BackgroundFetchResponse>) = 0;
  };

  virtual ~BackgroundFetchDelegate() {}

  virtual void DownloadUrl(
      const std::string& guid,
      const std::string& method,
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      const net::HttpRequestHeaders& headers) = 0;

  virtual void SetDelegateClient(base::WeakPtr<Client> client) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_H_
