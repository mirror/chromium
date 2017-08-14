// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_BACKGROUND_FETCH_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"

namespace content {
class BackgroundFetchRequestInfo;
class DownloadUrlParameters;

struct BackgroundFetchResponse;

// Must only be used on the UI thread.
class BackgroundFetchDelegate {
 public:
  enum FailureReason {
    // Used when the download has been aborted after reaching a threshold where
    // it was decided it is not worth attempting to start again. This could be
    // either due to a specific number of failed retry attempts or a specific
    // number of wasted bytes due to the download restarting.
    NETWORK,

    // Used when the download was not completed before the timeout.
    TIMEDOUT,

    // Used when the failure reason is unknown.
    UNKNOWN,
  };

  class Client {
   public:
    virtual void OnDownloadUpdated(const std::string& guid,
                                   uint64_t bytes_downloaded) = 0;

    virtual void OnDownloadFailed(const std::string& guid,
                                  FailureReason reason) = 0;

    virtual void OnDownloadSucceeded(const std::string& guid,
                                     const base::FilePath& path,
                                     uint64_t size) = 0;

    virtual void OnDownloadStarted(
        const std::string& guid,
        std::unique_ptr<const content::BackgroundFetchResponse>) = 0;
  };

  virtual ~BackgroundFetchDelegate() {}

  virtual void DownloadUrl(
      const std::string& guid,
      std::unique_ptr<DownloadUrlParameters> parameters,
      scoped_refptr<BackgroundFetchRequestInfo> request) = 0;

  virtual void OnDownloadStarted(
      const std::string& guid,
      std::unique_ptr<const content::BackgroundFetchResponse>) = 0;

  virtual void OnDownloadUpdated(const std::string& guid,
                                 uint64_t bytes_downloaded) = 0;

  virtual void OnDownloadFailed(const std::string& guid,
                                FailureReason reason) = 0;

  virtual void OnDownloadSucceeded(const std::string& guid,
                                   const base::FilePath& path,
                                   uint64_t size) = 0;

  virtual void SetDelegateClient(base::WeakPtr<Client> client) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BACKGROUND_FETCH_DELEGATE_H_
