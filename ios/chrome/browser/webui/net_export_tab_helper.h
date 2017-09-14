// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEBUI_NET_EXPORT_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEBUI_NET_EXPORT_TAB_HELPER_H_

#import "ios/web/public/web_state/web_state_user_data.h"

@protocol MailComposerPresenter;
@class ShowMailComposerCommand;

class NetExportTabHelper : public web::WebStateUserData<NetExportTabHelper> {
 public:
  ~NetExportTabHelper() override;

  static void CreateForWebState(web::WebState* web_state,
                                id<MailComposerPresenter> presenter);

  void ShowMailComposer(ShowMailComposerCommand* command);

 private:
  explicit NetExportTabHelper(id<MailComposerPresenter> presenter);
  __weak id<MailComposerPresenter> presenter_;
  DISALLOW_COPY_AND_ASSIGN(NetExportTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEBUI_NET_EXPORT_TAB_HELPER_H_
