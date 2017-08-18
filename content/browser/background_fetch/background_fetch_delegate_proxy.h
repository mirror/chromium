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
#include "content/public/browser/background_fetch_delegate.h"
#include "content/public/browser/browser_thread.h"

namespace content {

class BackgroundFetchJobController;
class BrowserContext;

// Proxy class for passing messages between BackgroundFetchJobControllers on the
// IO thread and BackgroundFetchDelegate on the UI thread.
class CONTENT_EXPORT BackgroundFetchDelegateProxy {
 public:
  explicit BackgroundFetchDelegateProxy(BrowserContext* browser_context);

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
  void OnDownloadFailed(std::string guid,
                        BackgroundFetchDelegate::FailureReason reason);

  void OnDownloadReceived(const std::string& guid,
                          BackgroundFetchDelegate::StartResult result);

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
