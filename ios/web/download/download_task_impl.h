// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_DOWNLOAD_DOWNLOAD_TASK_IMPL_H_
#define IOS_WEB_DOWNLOAD_DOWNLOAD_TASK_IMPL_H_

#include <string>

#import "ios/web/public/download/download_task.h"
#include "url/gurl.h"

@class NSURLSession;
namespace net {
class URLFetcherResponseWriter;
}

namespace web {

class WebState;

class DownloadTaskImpl : public DownloadTask {
 public:
  class Delegate {
   public:
    virtual void OnTaskUpdated(DownloadTaskImpl* task) = 0;
    virtual void OnTaskDestroyed(DownloadTaskImpl* task) = 0;
    virtual NSURLSession* CreateSession(NSString* identifier,
                                        id<NSURLSessionDataDelegate>) = 0;
    virtual ~Delegate() = default;
  };

  DownloadTaskImpl(const WebState* web_state,
                   const GURL& original_url,
                   const std::string& content_disposition,
                   int64_t total_bytes,
                   const std::string& mime_type,
                   NSString* identifier,
                   Delegate* delegate);

  const WebState* web_state() const { return web_state_; }

  // DownloadTask overrides:
  void Start() override;
  net::URLFetcherResponseWriter* GetResponseWriter() const override;
  void SetResponseWriter(
      std::unique_ptr<net::URLFetcherResponseWriter> writer) override;
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
  NSURLSession* CreateSession(NSString* identifier);

  const WebState* web_state_ = nullptr;
  GURL original_url_;
  std::string content_disposition_;
  std::string mime_type_;
  Delegate* delegate_ = nullptr;
  NSURLSession* session_ = nil;
  NSURLSessionTask* session_task_ = nil;

  bool is_started_ = false;
  bool is_done_ = false;
  bool error_code_ = 0;
  int64_t total_bytes_ = -1;
  int percent_complete_ = 0;

  std::unique_ptr<net::URLFetcherResponseWriter> writer_;
};

}  // namespace web

#endif  // IOS_WEB_DOWNLOAD_DOWNLOAD_TASK_IMPL_H_
