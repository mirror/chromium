// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_task_impl.h"

#import <Foundation/Foundation.h>

#include "net/base/filename_util.h"
#import "net/base/mac/url_conversions.h"
#include "net/url_request/url_fetcher_response_writer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

DownloadTaskImpl::DownloadTaskImpl(const WebState* web_state,
                                   const GURL& original_url,
                                   const std::string& content_disposition,
                                   const std::string& mime_type,
                                   NSURLSession* session,
                                   Delegate* delegate)
    : web_state_(web_state),
      original_url_(original_url),
      content_disposition_(content_disposition),
      mime_type_(mime_type),
      session_(session),
      delegate_(delegate) {}

DownloadTaskImpl::~DownloadTaskImpl() {
  if (delegate_)
    delegate_->OnTaskDestroyed(this);
}

void DownloadTaskImpl::Start() {
  if (is_started_)
    return;

  is_started_ = true;
  NSURL* url = net::NSURLWithGURL(GetOriginalUrl());
  NSURLSessionDataTask* data_task = [session_ dataTaskWithURL:url];
  DCHECK(data_task);
}

net::URLFetcherResponseWriter* DownloadTaskImpl::GetResponseWriter() const {
  return writer_.get();
}

void DownloadTaskImpl::SetResponseWriter(
    std::unique_ptr<net::URLFetcherResponseWriter> writer) {
  writer_ = std::move(writer);
}

NSString* DownloadTaskImpl::GetIndentifier() const {
  return session_.configuration.identifier;
}

const GURL& DownloadTaskImpl::GetOriginalUrl() const {
  return original_url_;
}

bool DownloadTaskImpl::IsDone() const {
  return is_done_;
}

int DownloadTaskImpl::GetErrorCode() const {
  return error_code_;
}

int64_t DownloadTaskImpl::GetTotalBytes() const {
  return total_bytes_;
}

int DownloadTaskImpl::GetPercentComplete() const {
  return percent_complete_;
}

std::string DownloadTaskImpl::GetContentDisposition() const {
  return content_disposition_;
}

std::string DownloadTaskImpl::GetMimeType() const {
  return mime_type_;
}

base::string16 DownloadTaskImpl::GetSuggestedFilename() const {
  return net::GetSuggestedFilename(GetOriginalUrl(), GetContentDisposition(),
                                   /*referrer_charset=*/"",
                                   /*suggested_name=*/"",
                                   /*mime_type=*/"",
                                   /*default_name=*/"document");
}

bool DownloadTaskImpl::GetTimeRemaining(base::Time*) const {
  return false;
}

}  // namespace web
