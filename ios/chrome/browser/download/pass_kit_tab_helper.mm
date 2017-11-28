// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/download/pass_kit_tab_helper.h"

#include <string>

#import <PassKit/PassKit.h>

#import "ios/chrome/browser/download/pass_kit_tab_helper_delegate.h"
#import "ios/web/public/download/download_task.h"
#include "net/url_request/url_fetcher_response_writer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(PassKitTabHelper);

PassKitTabHelper::PassKitTabHelper(web::WebState* web_state,
                                   id<PassKitTabHelperDelegate> delegate)
    : delegate_(delegate), weak_factory_(this) {
  DCHECK(delegate_);
}

PassKitTabHelper::~PassKitTabHelper() = default;

void PassKitTabHelper::CreateForWebState(
    web::WebState* web_state,
    id<PassKitTabHelperDelegate> delegate) {
  DCHECK(web_state);
  DCHECK(delegate);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(UserDataKey(), base::WrapUnique(new PassKitTabHelper(
                                              web_state, delegate)));
  }
}

void PassKitTabHelper::Download(std::unique_ptr<web::DownloadTask> task) {
  if (task_) {
    // The previous download task will be cancelled.
    task_->RemoveObserver(this);
  }

  task_ = std::move(task);
  task_->AddObserver(this);
  task_->Start(std::make_unique<net::URLFetcherStringWriter>());
}

void PassKitTabHelper::OnDownloadUpdated(const web::DownloadTask* task) {
  DCHECK_EQ(task_.get(), task);
  if (task->IsDone()) {
    std::string data = task->GetResponseWriter()->AsStringWriter()->data();
    NSData* nsdata = [NSData dataWithBytes:data.c_str() length:data.size()];
    NSError* error = nil;
    PKPass* pass = [[PKPass alloc] initWithData:nsdata error:&error];
    [delegate_ passKitTabHelper:this presentDialogForPass:pass error:error];
    // TODO report UMA for error code.
    task_->RemoveObserver(this);
    task_.reset();
  }
}
