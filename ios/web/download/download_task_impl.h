// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_DOWNLOAD_DOWNLOAD_TASK_IMPL_H_
#define IOS_WEB_DOWNLOAD_DOWNLOAD_TASK_IMPL_H_

#include <string>

#include "base/memory/weak_ptr.h"
#import "ios/web/public/download/download_task.h"
#include "url/gurl.h"

@class NSURLSession;

namespace net {
class URLFetcherResponseWriter;
}

namespace web {

class WebState;

// Implements DownloadTask interface. Uses background NSURLSession as
// implementation. Must be used on UI thread.
class DownloadTaskImpl : public DownloadTask {
 public:
  class Delegate {
   public:
    // Called when download task has started, downloaded a chunk of data or
    // the download has been completed.
    virtual void OnTaskUpdated(DownloadTaskImpl* task) = 0;

    // Called when downaload task is about to be destroyed. Delegate should
    // remove all references to the given DownloadTask and stop using it.
    virtual void OnTaskDestroyed(DownloadTaskImpl* task) = 0;

    // Creates background NSURLSession with given |identifier|, |delegate| and
    // |delegate_queue|.
    virtual NSURLSession* CreateSession(NSString* identifier,
                                        id<NSURLSessionDataDelegate> delegate,
                                        NSOperationQueue* delegate_queue) = 0;
    virtual ~Delegate() = default;
  };

  // Constructs a new DownloadTaskImpl objects. |web_state|, |identifier| and
  // |delegate| must not be null.
  DownloadTaskImpl(const WebState* web_state,
                   const GURL& original_url,
                   const std::string& content_disposition,
                   int64_t total_bytes,
                   const std::string& mime_type,
                   NSString* identifier,
                   Delegate* delegate);

  const WebState* web_state() const { return web_state_; }

  // Stops the download operation and clears the delegate.
  void ShutDown();

  // DownloadTask overrides:
  void Start() override;
  void SetResponseWriter(
      std::unique_ptr<net::URLFetcherResponseWriter> writer) override;
  net::URLFetcherResponseWriter* GetResponseWriter() const override;
  NSString* GetIndentifier() const override;
  const GURL& GetOriginalUrl() const override;
  bool IsDone() const override;
  int GetErrorCode() const override;
  int64_t GetTotalBytes() const override;
  int GetPercentComplete() const override;
  std::string GetContentDisposition() const override;
  std::string GetMimeType() const override;
  base::string16 GetSuggestedFilename() const override;

 protected:
  ~DownloadTaskImpl() override;

 private:
  // Creates background NSURLSession with given |identifier|.
  NSURLSession* CreateSession(NSString* identifier);

  // Asynchronously returns cookies for WebState associated with this task on
  // iOS 11. Synchronously returns an empty array on iOS 10 and lower. Must be
  // called on UI thread.
  void GetCookies(base::Callback<void(NSArray<NSHTTPCookie*>*)>);

  // Asynchronously returns cookies for WebState associated with this task. Must
  // be called on IO thread.
  void GetWKCookies(base::Callback<void(NSArray<NSHTTPCookie*>*)> callback)
      API_AVAILABLE(ios(11.0));

  // Starts the download with given cookies.
  void StartWithCookies(NSArray<NSHTTPCookie*>* cookies);

  // Called when download task was updated.
  void OnDownloadUpdated();

  // Called when download was completed and the data writing was finished.
  void OnDownloadFinished(int error_code);

  // Back up corresponding public methods of DownloadTask interface.
  std::unique_ptr<net::URLFetcherResponseWriter> writer_;
  GURL original_url_;
  bool is_done_ = false;
  int error_code_ = 0;
  int64_t total_bytes_ = -1;
  int percent_complete_ = -1;
  std::string content_disposition_;
  std::string mime_type_;

  const WebState* web_state_ = nullptr;
  bool is_started_ = false;
  Delegate* delegate_ = nullptr;
  NSURLSession* session_ = nil;
  NSURLSessionTask* session_task_ = nil;

  base::WeakPtrFactory<DownloadTaskImpl> weak_factory_;
};

}  // namespace web

#endif  // IOS_WEB_DOWNLOAD_DOWNLOAD_TASK_IMPL_H_
