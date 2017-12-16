// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_TAB_HELPER_H_

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class ExternalAppsLaunchPolicyDecider;
class GURL;

// A customized external app launcher that optionally shows a modal
// confirmation dialog before switching context to an external application.
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
  BOOL RequestToOpenUrl(const GURL& url,
                        const GURL& source_page_url,
                        BOOL link_clicked);

 private:
  friend class web::WebStateUserData<ExternalAppLauncherTabHelper>;
  explicit ExternalAppLauncherTabHelper(web::WebState* web_state);

  void HandleRepeatedAttemptsToLaunch(const GURL& url,
                                      const GURL& source_page_url,
                                      BOOL allowed);

  // Used to check for repeated launches and provide policy for launching apps.
  ExternalAppsLaunchPolicyDecider* policy_decider_;

  // Returns whether there is a prompt shown by |requestToOpenURL| or not.
  BOOL prompt_active_;

  // Weak pointer to self. Must be last member to ensure it is destroyed last.
  base::WeakPtrFactory<ExternalAppLauncherTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalAppLauncherTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_TAB_HELPER_H_
