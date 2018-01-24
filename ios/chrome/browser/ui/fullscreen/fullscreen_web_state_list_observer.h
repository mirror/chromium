// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_WEB_STATE_LIST_OBSERVER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_WEB_STATE_LIST_OBSERVER_H_

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_impl.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"
#include "ios/web/public/web_state/web_state_observer.h"

class FullscreenModel;

// A WebStateListObserver that creates WebStateObservers that update a
// FullscreenModel for navigation events.
class FullscreenWebStateListObserver : public WebStateListObserver {
 public:
  // Constructor for an observer for |web_state_list| that updates |model|.
  // |controller| is used to create ScopedFullscreenDisablers for WebState
  // navigation events that require the toolbar to be visible.
  FullscreenWebStateListObserver(FullscreenControllerImpl* controller);
  ~FullscreenWebStateListObserver() override;

  // Stops observing the the WebStateList.
  void Disconnect();

 private:
  // WebStateListObserver:
  void WebStateReplacedAt(WebStateList* web_state_list,
                          web::WebState* old_web_state,
                          web::WebState* new_web_state,
                          int index) override;
  void WebStateActivatedAt(WebStateList* web_state_list,
                           web::WebState* old_web_state,
                           web::WebState* new_web_state,
                           int active_index,
                           int reason) override;

  // FullscreenControllerImpl accessors.
  FullscreenModel* model() const { return controller_->model(); }
  WebStateList* web_state_list() const { return controller_->web_state_list(); }

  // The FullscreenController passed on initialization.
  FullscreenControllerImpl* controller_;
  // The observer for the active WebState.
  FullscreenWebStateObserver web_state_observer_;
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_WEB_STATE_LIST_OBSERVER_H_
