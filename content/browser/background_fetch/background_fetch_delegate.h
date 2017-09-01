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

// Interface for launching background fetches. Implementing classes would
// generally interface with the DownloadService or DownloadManager.
// Must only be used on the UI thread and generally expected to be called by the
// BackgroundFetchDelegateProxy.
class BackgroundFetchDelegate {
 public:
  // Client interface that a BackgroundFetchDelegate would use to signal the
  // progress of a background fetch.
  class Client {
   public:
    virtual void OnDownloadStarted(
        const std::string& guid,
        std::unique_ptr<content::BackgroundFetchResponse>) = 0;

    virtual void OnDownloadUpdated(const std::string& guid,
                                   uint64_t bytes_downloaded) = 0;

    virtual void OnDownloadComplete(
        const std::string& guid,
        std::unique_ptr<BackgroundFetchResult> result) = 0;
  };

  virtual ~BackgroundFetchDelegate() {}

  virtual void DownloadUrl(
      const std::string& guid,
      const std::string& method,
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      const net::HttpRequestHeaders& headers) = 0;

  // Set the client that the delegate should communicate changes to.
  virtual void SetDelegateClient(base::WeakPtr<Client> client) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_H_
