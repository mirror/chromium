// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ios/web/public/download/download_task_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol PassKitTabHelperDelegate;
namespace web {
class DownloadTask;
}  // namespace web

class PassKitTabHelper : public web::WebStateUserData<PassKitTabHelper>,
                         public web::DownloadTaskObserver {
 public:
  ~PassKitTabHelper() override;

  // Creates TabHelper. |delegate| is not retained by TabHelper and must not be
  // null.
  static void CreateForWebState(web::WebState* web_state,
                                id<PassKitTabHelperDelegate> delegate);

  void Download(std::unique_ptr<web::DownloadTask> task);

 private:
  // web::DownloadTaskObserver overrides:
  void OnDownloadUpdated(const web::DownloadTask* task) override;

  PassKitTabHelper(web::WebState* web_state,
                   id<PassKitTabHelperDelegate> delegate);

  __weak id<PassKitTabHelperDelegate> delegate_ = nil;
  std::unique_ptr<web::DownloadTask> task_;
  base::WeakPtrFactory<PassKitTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PassKitTabHelper);
};

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_PASS_KIT_TAB_HELPER_H_
