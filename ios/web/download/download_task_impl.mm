// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_task_impl.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#import "base/mac/bind_objc_block.h"
#import "ios/web/net/wk_cookie_util.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_thread.h"
#import "ios/web/web_state/error_translation_util.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "net/base/filename_util.h"
#include "net/base/io_buffer.h"
#import "net/base/mac/url_conversions.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_fetcher_response_writer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWURLSessionDelegate : NSObject<NSURLSessionDataDelegate>
- (instancetype)
initWithUpdateBlock:(void (^)(NSURLSessionTask*, int))updateBlock
          dataBlock:(void (^)(const void*, NSUInteger length))dataBlock;
- (instancetype)init NS_UNAVAILABLE;
@end

@implementation CRWURLSessionDelegate {
  void (^_updateBlock)(NSURLSessionTask*, int);
  void (^_dataBlock)(const void*, NSUInteger);
}

- (instancetype)
initWithUpdateBlock:(void (^)(NSURLSessionTask*, int error))updateBlock
          dataBlock:(void (^)(const void*, NSUInteger length))dataBlock {
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

  int errorCode = 0;
  if (error) {
    if (!web::GetNetErrorFromIOSErrorCode(error.code, &errorCode)) {
      errorCode = net::ERR_FAILED;
    }
  }

  _updateBlock(task, errorCode);
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionDataTask*)task
    didReceiveData:(NSData*)data {
  [data enumerateByteRangesUsingBlock:^(const void* _Nonnull bytes,
                                        NSRange byteRange, BOOL* _Nonnull) {
    _dataBlock(bytes, byteRange.length);
  }];
  _updateBlock(task, 0);
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
      total_bytes_(total_bytes),
      weak_factory_(this) {
  CHECK(session_);
  CHECK(delegate_);
}

DownloadTaskImpl::~DownloadTaskImpl() {
  [session_task_ cancel];
  delegate_->OnTaskDestroyed(this);
}

void DownloadTaskImpl::Start() {
  if (is_started_)
    return;
  is_started_ = true;
  GetCookies(base::Bind(&DownloadTaskImpl::StartWithCookies,
                        weak_factory_.GetWeakPtr()));
}

void DownloadTaskImpl::StartWithCookies(NSArray<NSHTTPCookie*>* cookies) {
  DCHECK(writer_);

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
      initWithUpdateBlock:^(NSURLSessionTask* task, int error_code) {
        if (!weak_factory_.GetWeakPtr())
          return;

        double progress = task.countOfBytesExpectedToReceive
                              ? static_cast<double>(task.countOfBytesReceived) /
                                    task.countOfBytesExpectedToReceive
                              : 1;

        percent_complete_ = progress * 100;
        total_bytes_ = task.countOfBytesExpectedToReceive;
        error_code_ = error_code;
        if (task.state == NSURLSessionTaskStateCompleted) {
          writer_->Finish(error_code, base::Bind(&NoOp));
          is_done_ = true;
          session_task_ = nil;
        }
        delegate_->OnTaskUpdated(this);
      }
      dataBlock:^(const void* data, NSUInteger length) {
        if (!weak_factory_.GetWeakPtr())
          return;

        scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(length));
        // TODO: how to avoid copying the memory?
        memcpy(buffer->data(), data, length);
        writer_->Write(buffer.get(), length, base::Bind(&NoOp));
      }];
  return delegate_->CreateSession(identifier, session_delegate);
}

void DownloadTaskImpl::GetCookies(
    base::Callback<void(NSArray<NSHTTPCookie*>*)> callback) {
  if (@available(iOS 11, *)) {
    web::WebThread::PostTask(
        web::WebThread::IO, FROM_HERE, base::BindBlockArc(^{
          WKHTTPCookieStore* store =
              web::WKCookieStoreForBrowserState(web_state_->GetBrowserState());
          [store getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
            web::WebThread::PostTask(web::WebThread::UI, FROM_HERE,
                                     base::Bind(callback, cookies));
          }];
        }));

    return;
  }

  web::WebThread::PostTask(web::WebThread::UI, FROM_HERE,
                           base::Bind(callback, [NSArray array]));
}
}  // namespace web
