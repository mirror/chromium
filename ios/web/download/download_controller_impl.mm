// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_controller_impl.h"

#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/download/download_controller_observer.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char kDownloadControllerKey = 0;
}  // namespace

namespace web {

// static
DownloadController* DownloadController::FromBrowserState(
    BrowserState* browser_state) {
  DCHECK(browser_state);
  if (!browser_state->GetUserData(&kDownloadControllerKey)) {
    browser_state->SetUserData(&kDownloadControllerKey,
                               base::MakeUnique<DownloadControllerImpl>());
  }
  return static_cast<DownloadControllerImpl*>(
      browser_state->GetUserData(&kDownloadControllerKey));
}

DownloadControllerImpl::DownloadControllerImpl() = default;
DownloadControllerImpl::~DownloadControllerImpl() = default;

void DownloadControllerImpl::CreateDownloadTask(
    const WebState* web_state,
    NSString* identifier,
    const GURL& original_url,
    const std::string& content_disposition,
    const std::string& mime_type) {
  scoped_refptr<DownloadTaskImpl> task(
      new DownloadTaskImpl(web_state, original_url, content_disposition,
                           mime_type, identifier, this));

  for (auto& observer : observers_)
    observer.OnDownloadCreated(task->web_state(), task);
}

void DownloadControllerImpl::AddObserver(DownloadControllerObserver* observer) {
  DCHECK(!observers_.HasObserver(observer));
  observers_.AddObserver(observer);
}

void DownloadControllerImpl::RemoveObserver(
    DownloadControllerObserver* observer) {
  DCHECK(observers_.HasObserver(observer));
  observers_.RemoveObserver(observer);
}

void DownloadControllerImpl::OnTaskUpdated(DownloadTaskImpl* task) {
  for (auto& observer : observers_)
    observer.OnDownloadUpdated(task->web_state(), task);
}

void DownloadControllerImpl::OnTaskDestroyed(DownloadTaskImpl* task) {
  for (auto& observer : observers_)
    observer.OnDownloadDestroyed(task->web_state(), task);
}

NSURLSession* DownloadControllerImpl::CreateSession(
    NSString* identifier,
    id<NSURLSessionDelegate> delegate) {
  NSURLSessionConfiguration* configuration = [NSURLSessionConfiguration
      backgroundSessionConfigurationWithIdentifier:identifier];
  return [NSURLSession sessionWithConfiguration:configuration
                                       delegate:delegate
                                  delegateQueue:nil];
}

}  // namespace web
