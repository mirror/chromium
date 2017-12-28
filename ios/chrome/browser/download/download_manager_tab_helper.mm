// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/download/download_manager_tab_helper.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/download/download_manager_tab_helper_delegate.h"
#import "ios/web/public/download/download_task.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(DownloadManagerTabHelper);

DownloadManagerTabHelper::DownloadManagerTabHelper(
    web::WebState* web_state,
    id<DownloadManagerTabHelperDelegate> delegate)
    : web_state_(web_state), delegate_(delegate) {
  DCHECK(web_state_);
}

DownloadManagerTabHelper::~DownloadManagerTabHelper() {
  if (task_)
    task_->RemoveObserver(this);
}

void DownloadManagerTabHelper::CreateForWebState(
    web::WebState* web_state,
    id<DownloadManagerTabHelperDelegate> delegate) {
  DCHECK(web_state);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(),
        base::WrapUnique(new DownloadManagerTabHelper(web_state, delegate)));
  }
}

void DownloadManagerTabHelper::Download(
    std::unique_ptr<web::DownloadTask> task) {
  task_ = std::move(task);
  task_->AddObserver(this);
  [delegate_ downloadManagerTabHelper:this didRequestDownload:task_.get()];
}

void DownloadManagerTabHelper::OnDownloadUpdated(web::DownloadTask* task) {
  DCHECK_EQ(task, task_.get());
  switch (task->GetState()) {
    case web::DownloadTask::State::kCancelled:
      task_->RemoveObserver(this);
      task_ = nullptr;
      break;
    case web::DownloadTask::State::kInProgress:
    case web::DownloadTask::State::kComplete:
      [delegate_ downloadManagerTabHelper:this didUpdateDownload:task_.get()];
      break;
    case web::DownloadTask::State::kNotStarted:
      // OnDownloadUpdated should not be called with this state.
      NOTREACHED();
  }
}
