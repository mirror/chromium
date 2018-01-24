// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_list_observer.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_view_scroll_view_replacement_util.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenWebStateListObserver::FullscreenWebStateListObserver(
    FullscreenControllerImpl* controller)
    : controller_(controller), web_state_observer_(controller) {
  web_state_list()->AddObserver(this);
  web_state_observer_.SetWebState(web_state_list()->GetActiveWebState());
}

FullscreenWebStateListObserver::~FullscreenWebStateListObserver() {
  // Disconnect() should be called before destruction.
  DCHECK(!controller_);
}

void FullscreenWebStateListObserver::Disconnect() {
  web_state_list()->RemoveObserver(this);
  web_state_observer_.SetWebState(nullptr);
  controller_ = nullptr;
}

void FullscreenWebStateListObserver::WebStateReplacedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int index) {
  if (new_web_state == web_state_list->GetActiveWebState()) {
    // Reset the model if the active WebState is replaced.
    web_state_observer_.SetWebState(new_web_state);
    model()->ResetForNavigation();
    if (new_web_state) {
      UpdateFullscreenWebViewProxyForReplacedScrollView(
          new_web_state->GetWebViewProxy(), model());
    }
  }
}

void FullscreenWebStateListObserver::WebStateActivatedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int active_index,
    int reason) {
  web_state_observer_.SetWebState(new_web_state);
}
