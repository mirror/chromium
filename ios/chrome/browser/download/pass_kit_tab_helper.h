// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "ios/web/public/download/download_task_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol PassKitTabHelperDelegate;
namespace web {
class DownloadTask;
class WebState;
}  // namespace web

class PassKitTabHelper : public web::WebStateUserData<PassKitTabHelper>,
                         public web::DownloadTaskObserver {
 public:
  ~PassKitTabHelper() override;

  // Creates TabHelper. |delegate| is not retained by TabHelper and must not be
  // null.
  static void CreateForWebState(web::WebState* web_state,
                                id<PassKitTabHelperDelegate> delegate);

  // Asynchronously downloads pkpass file using the given |task|. Asks delegate
  // to present "Add pkpass" dialog when the download is complete.
  void Download(std::unique_ptr<web::DownloadTask> task);

 private:
  // web::DownloadTaskObserver overrides:
  void OnDownloadUpdated(web::DownloadTask* task) override;

  PassKitTabHelper(web::WebState* web_state,
                   id<PassKitTabHelperDelegate> delegate);

  __weak id<PassKitTabHelperDelegate> delegate_ = nil;
  // Set of unfinished download tasks.
  std::set<std::unique_ptr<web::DownloadTask>> tasks_;
  web::WebState* web_state_;

  DISALLOW_COPY_AND_ASSIGN(PassKitTabHelper);
};

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_H_
