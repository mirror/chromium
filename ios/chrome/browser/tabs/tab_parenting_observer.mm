// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_parenting_observer.h"

#include "ios/chrome/browser/tab_parenting_global_observer.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
void OnWebStateParented(TabModel* tab_model, web::WebState* web_state) {
  TabParentingGlobalObserver::GetInstance()->OnTabParented(web_state);
  Tab* tab = LegacyTabHelper::GetTabForWebState(web_state);
  web_state->SetWebUsageEnabled(tab_model.webUsageEnabled);
  [tab fetchFavicon];
}
}  // namespace

TabParentingObserver::TabParentingObserver(TabModel* tab_model)
    : tab_model_(tab_model) {
  DCHECK(tab_model_);
}

TabParentingObserver::~TabParentingObserver() = default;

void TabParentingObserver::WebStateInsertedAt(WebStateList* web_state_list,
                                              web::WebState* web_state,
                                              int index,
                                              bool activating) {
  OnWebStateParented(tab_model_, web_state);

  // Avoid artificially extending the lifetime of oldTab until the global
  // autoreleasepool is purged.
  @autoreleasepool {
    Tab* tab = LegacyTabHelper::GetTabForWebState(web_state);
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kTabModelNewTabWillOpenNotification
                      object:tab_model_
                    userInfo:@{
                      kTabModelTabKey : tab,
                      kTabModelOpenInBackgroundKey : @(!activating),
                    }];
  }
}

void TabParentingObserver::WebStateReplacedAt(WebStateList* web_state_list,
                                              web::WebState* old_web_state,
                                              web::WebState* new_web_state,
                                              int index) {
  OnWebStateParented(tab_model_, new_web_state);
}
