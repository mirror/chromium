// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_TAB_HELPER_H_

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class ExternalAppLauncher;
class GURL;

class ExternalAppLauncherTabHelper
    : public web::WebStateUserData<ExternalAppLauncherTabHelper> {
 public:
  ~ExternalAppLauncherTabHelper() override;

  // Requests to open URL in an external application.
  // The method checks if the application for |gURL| has been opened repeatedly
  // by the |sourcePageURL| page in a short time frame, in that case a prompt
  // will appear to the user with an option to block the application from
  // launching. Then the method also checks for user interaction and for schemes
  // that require special handling (eg. facetime, mailto) and may present the
  // user with a confirmation dialog to open the application. If there is no
  // such application available or it's not possible to open the application the
  // method returns NO.
  BOOL requestToOpenURL(const GURL& gURL,
                        const GURL& sourcePageURL,
                        BOOL linkClicked);

 private:
  friend class web::WebStateUserData<ExternalAppLauncherTabHelper>;
  explicit ExternalAppLauncherTabHelper(web::WebState* web_state);

  ExternalAppLauncher* launcher_;

  DISALLOW_COPY_AND_ASSIGN(ExternalAppLauncherTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_TAB_HELPER_H_
