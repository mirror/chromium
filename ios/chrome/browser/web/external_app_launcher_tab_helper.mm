// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_app_launcher_tab_helper.h"

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/external_app/external_app_presenter.h"
#import "ios/chrome/browser/web/external_apps_launch_policy_decider.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(ExternalAppLauncherTabHelper);

ExternalAppLauncherTabHelper::ExternalAppLauncherTabHelper(
    web::WebState* web_state)
    : policy_decider_([[ExternalAppsLaunchPolicyDecider alloc] init]),
      weak_factory_(this) {}

ExternalAppLauncherTabHelper::~ExternalAppLauncherTabHelper() = default;

void ExternalAppLauncherTabHelper::SetPresenter(
    id<ExternalAppPresenter> presenter) {
  presenter_ = presenter;
}

bool ExternalAppLauncherTabHelper::RequestToOpenUrl(const GURL& url,
                                                    const GURL& source_page_url,
                                                    bool link_tapped) {
  if (!url.is_valid() || !url.has_scheme())
    return false;

  // Don't open external application if chrome is not active.
  if ([[UIApplication sharedApplication] applicationState] !=
      UIApplicationStateActive) {
    return false;
  }

  // Don't try to open external application if a prompt is already active.
  if (is_prompt_active_)
    return false;

  [policy_decider_ didRequestLaunchExternalAppURL:url
                                fromSourcePageURL:source_page_url];
  ExternalAppLaunchPolicy policy =
      [policy_decider_ launchPolicyForURL:url
                        fromSourcePageURL:source_page_url];
  switch (policy) {
    case ExternalAppLaunchPolicyBlock: {
      return false;
    }
    case ExternalAppLaunchPolicyAllow: {
      return [presenter_ openURL:url linkTapped:link_tapped];
    }
    case ExternalAppLaunchPolicyPrompt: {
      is_prompt_active_ = true;
      base::WeakPtr<ExternalAppLauncherTabHelper> weak_this =
          weak_factory_.GetWeakPtr();
      [presenter_ showAlertOfRepeatedLaunchesWithCompletionHandler:^(
                      BOOL user_allowed) {
        if (!weak_this.get())
          return;
        if (user_allowed) {
          // By confirming that user wants to launch the
          // application, there is no need to check for
          // |link_tapped|.
          [presenter_ openURL:url linkTapped:YES];
        } else {
          // TODO(crbug.com/674649): Once non modal
          // dialogs are implemented, update this to
          // always prompt instead of blocking the app.
          [policy_decider_ blockLaunchingAppURL:url
                              fromSourcePageURL:source_page_url];
        }
        is_prompt_active_ = false;
      }];
      return true;
    }
  }
}
