// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_PROXY_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_PROXY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/background_fetch/background_fetch_request_info.h"
#include "content/public/browser/browser_thread.h"

namespace net {
class URLRequestContextGetter;
}

namespace content {

class BackgroundFetchJobController;
struct BackgroundFetchResponse;
class BrowserContext;

// Proxy class for passing messages between BackgroundFetchJobControllers on the
// IO thread and BackgroundFetchDelegate on the UI thread.
// TODO(delphick): Create BackgroundFetchDelegate.
class CONTENT_EXPORT BackgroundFetchDelegateProxy {
 public:
  // Failures that happen after the download has already started.
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

  // Results returned immediately after attempting to start the fetch (but
  // usually before reaching the network).
  enum StartResult {
    // The download is accepted and persisted.
    ACCEPTED,

    // The DownloadService has too many downloads.  Back off and retry.
    BACKOFF,

    // Failed to create the download.  The guid is already in use.
    UNEXPECTED_GUID,

    // The DownloadService was unable to accept and persist this download due to
    // an internal error like the underlying DB store failing to write to disk.
    INTERNAL_ERROR,
  };

  BackgroundFetchDelegateProxy(
      BrowserContext* browser_context,
      scoped_refptr<net::URLRequestContextGetter> request_context);

  ~BackgroundFetchDelegateProxy();

  // Requests that the download manager start fetching |request|.
  // Should only be called from the BackgroundFetchJobController (on the IO
  // thread).
  void StartRequest(BackgroundFetchJobController* job_controller,
                    scoped_refptr<BackgroundFetchRequestInfo> request);

  // Updates the representation of this Background Fetch in the user interface
  // to match the given |title|.
  // Should only be called from the BackgroundFetchJobController (on the IO
  // thread).
  void UpdateUI(const std::string& title);

  // Immediately aborts this Background Fetch by request of the developer.
  // Should only be called from the BackgroundFetchJobController (on the IO
  // thread).
  void Abort();

 private:
  class Core;

  // Called when the given download identified by |guid| has been completed
  // successfully. Should only be called on the IO thread.
  void OnDownloadSucceeded(std::string guid,
                           base::FilePath path,
                           uint64_t size);

  // Called when the given download identified by |guid| has failed. Should only
  // be called on the IO thread.
  void OnDownloadFailed(std::string guid, FailureReason reason);

  void OnRequestReceived(const std::string& guid, StartResult result);

  // Should only be called from the BackgroundFetchDelegate (on the IO thread).
  void DidStartRequest(const std::string& guid,
                       std::unique_ptr<const BackgroundFetchResponse> response);

  std::unique_ptr<Core, BrowserThread::DeleteOnUIThread> ui_core_;
  base::WeakPtr<Core> ui_core_ptr_;

  // Map from DownloadService GUIDs to the RequestInfo and the JobController
  // that started the download.
  std::unordered_map<std::string,
                     std::pair<scoped_refptr<BackgroundFetchRequestInfo>,
                               base::WeakPtr<BackgroundFetchJobController>>>
      controller_map_;

  base::WeakPtrFactory<BackgroundFetchDelegateProxy> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateProxy);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_PROXY_H_
