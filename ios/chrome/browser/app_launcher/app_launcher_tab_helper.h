// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_APP_LAUNCHER_APP_LAUNCHER_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_APP_LAUNCHER_APP_LAUNCHER_TAB_HELPER_H_

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol AppLaunching;
@class ExternalAppsLaunchPolicyDecider;
class GURL;

// A tab helper that handles requests to launch another application.
class AppLauncherTabHelper
    : public web::WebStateUserData<AppLauncherTabHelper> {
 public:
  ~AppLauncherTabHelper() override;

  // Sets a weak reference to the |app_launcher| which can launch applications.
  void SetAppLauncher(id<AppLaunching> app_launcher);

  // Requests to open the application with |url|.
  // The method checks if the application for |url| has been opened repeatedly
  // by the |source_page_url| page in a short time frame, in that case a prompt
  // will appear to the user with an option to block the application from
  // launching. Then the method also checks for user interaction and for schemes
  // that require special handling (eg. facetime, mailto) and may present the
  // user with a confirmation dialog to open the application. If there is no
  // such application available or it's not possible to open the application the
  // method returns NO.
  bool RequestToLaunchAppWithUrl(const GURL& url,
                                 const GURL& source_page_url,
                                 bool link_tapped);

 private:
  friend class web::WebStateUserData<AppLauncherTabHelper>;
  explicit AppLauncherTabHelper(web::WebState* web_state);

  // Used to launch apps.
  __weak id<AppLaunching> app_launcher_ = nil;

  // Used to check for repeated launches and provide policy for launching apps.
  ExternalAppsLaunchPolicyDecider* policy_decider_ = nil;

  // Returns whether there is a prompt shown by |RequestToOpenUrl| or not.
  bool is_prompt_active_ = false;

  // Must be last member to ensure it is destroyed last.
  base::WeakPtrFactory<AppLauncherTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppLauncherTabHelper);
};

#endif  // IOS_CHROME_BROWSER_APP_LAUNCHER_APP_LAUNCHER_TAB_HELPER_H_
