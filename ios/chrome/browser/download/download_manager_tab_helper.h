// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_TAB_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "ios/web/public/download/download_task_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol DownloadManagerTabHelperDelegate;
namespace web {
class DownloadTask;
class WebState;
}  // namespace web

// TabHelper which manages a single file download.
class DownloadManagerTabHelper
    : public web::WebStateUserData<DownloadManagerTabHelper>,
      public web::DownloadTaskObserver {
 public:
  ~DownloadManagerTabHelper() override;

  // Creates TabHelper. |delegate| is not retained by TabHelper. |web_state|
  // must not be null.
  static void CreateForWebState(web::WebState* web_state,
                                id<DownloadManagerTabHelperDelegate> delegate);

  // Asynchronously downloads a file using the given |task|.
  virtual void Download(std::unique_ptr<web::DownloadTask> task);

 protected:
  // Allow subclassing from DownloadManagerTabHelper for testing purposes.
  DownloadManagerTabHelper(web::WebState* web_state,
                           id<DownloadManagerTabHelperDelegate> delegate);

 private:
  // web::DownloadTaskObserver overrides:
  void OnDownloadUpdated(web::DownloadTask* task) override;

  web::WebState* web_state_;
  __weak id<DownloadManagerTabHelperDelegate> delegate_ = nil;
  std::unique_ptr<web::DownloadTask> task_;

  DISALLOW_COPY_AND_ASSIGN(DownloadManagerTabHelper);
};

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_TAB_HELPER_H_
