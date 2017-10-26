// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_task_impl.h"

#import <Foundation/Foundation.h>

#include "net/base/filename_util.h"
#include "net/base/io_buffer.h"
#import "net/base/mac/url_conversions.h"
#include "net/url_request/url_fetcher_response_writer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWURLSessionDelegate : NSObject<NSURLSessionDataDelegate>
- (instancetype)initWithUpdateBlock:(void (^)(NSURLSessionTask*))updateBlock
                          dataBlock:(void (^)(const void*,
                                              NSUInteger length))dataBlock;
- (instancetype)init NS_UNAVAILABLE;
@end

@implementation CRWURLSessionDelegate {
  void (^_updateBlock)(NSURLSessionTask*);
  void (^_dataBlock)(const void*, NSUInteger);
}

- (instancetype)initWithUpdateBlock:(void (^)(NSURLSessionTask*))updateBlock
                          dataBlock:(void (^)(const void*,
                                              NSUInteger length))dataBlock {
  DCHECK(updateBlock);
  DCHECK(dataBlock);
  self = [super init];
  if (self) {
    _updateBlock = updateBlock;
    _dataBlock = dataBlock;
  }
  return self;
}

- (void)URLSession:(NSURLSession*)session
    didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge
      completionHandler:
          (void (^)(NSURLSessionAuthChallengeDisposition disposition,
                    NSURLCredential* _Nullable credential))completionHandler {
  // TODO: implement
  completionHandler(NSURLSessionAuthChallengeRejectProtectionSpace, nil);
}

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionTask*)task
    didCompleteWithError:(nullable NSError*)error {
  if (error.code == NSURLErrorCancelled) {
    // DownloadTaskImpl was destructed.
    return;
  }
  _updateBlock(task);
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionDataTask*)task
    didReceiveData:(NSData*)data {
  [data enumerateByteRangesUsingBlock:^(const void* _Nonnull bytes,
                                        NSRange byteRange, BOOL* _Nonnull) {
    _dataBlock(bytes, byteRange.length);
  }];
  _updateBlock(task);
}

@end

namespace web {

namespace {
void NoOp(int) {}
}  // namespace

DownloadTaskImpl::DownloadTaskImpl(const WebState* web_state,
                                   const GURL& original_url,
                                   const std::string& content_disposition,
                                   int64_t total_bytes,
                                   const std::string& mime_type,
                                   NSString* identifier,
                                   Delegate* delegate)
    : web_state_(web_state),
      original_url_(original_url),
      content_disposition_(content_disposition),
      mime_type_(mime_type),
      delegate_(delegate),
      session_(CreateSession(identifier)),
      total_bytes_(total_bytes) {
  CHECK(session_);
  CHECK(delegate_);
}

DownloadTaskImpl::~DownloadTaskImpl() {
  [session_task_ cancel];
  delegate_->OnTaskDestroyed(this);
}

void DownloadTaskImpl::Start() {
  DCHECK(writer_);
  if (is_started_)
    return;

  is_started_ = true;
  NSURL* url = net::NSURLWithGURL(GetOriginalUrl());
  session_task_ = [session_ dataTaskWithURL:url];
  [session_task_ resume];
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

NSURLSession* DownloadTaskImpl::CreateSession(NSString* identifier) {
  id<NSURLSessionDataDelegate> session_delegate = [[CRWURLSessionDelegate alloc]
      initWithUpdateBlock:^(NSURLSessionTask* task) {
        double progress = task.countOfBytesExpectedToReceive
                              ? static_cast<double>(task.countOfBytesReceived) /
                                    task.countOfBytesExpectedToReceive
                              : 1;

        percent_complete_ = progress * 100;
        total_bytes_ = task.countOfBytesExpectedToReceive;
        if (task.state == NSURLSessionTaskStateCompleted) {
          writer_->Finish(0, base::Bind(&NoOp));
          is_done_ = true;
          session_task_ = nil;
        }
        delegate_->OnTaskUpdated(this);
      }
      dataBlock:^(const void* data, NSUInteger length) {
        scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(length));
        // TODO: how to avoid copying the memory?
        memcpy(buffer->data(), data, length);
        writer_->Write(buffer.get(), length, base::Bind(&NoOp));
      }];
  return delegate_->CreateSession(identifier, session_delegate);
}

}  // namespace web
