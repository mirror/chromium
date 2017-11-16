// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#import "ios/chrome/browser/ui/browser_list/browser_user_data.h"

@class FullscreenBroadcastForwarder;
@protocol FullscreenBroadcastReceiving;
class FullscreenControllerObserver;
class FullscreenControllerObserverManager;
class FullscreenModel;
class FullscreenWebStateListObserver;

// An object that observes scrolling events in the main content area and
// calculates how much of the toolbar should be visible as a result.  When the
// user scrolls down the screen, the toolbar should be hidden to allow more of
// the page's content to be visible.
class FullscreenController : public BrowserUserData<FullscreenController> {
 public:
  ~FullscreenController() override;

  // Adds and removes FullscreenControllerObservers.
  void AddObserver(FullscreenControllerObserver* observer);
  void RemoveObserver(FullscreenControllerObserver* observer);

  // FullscreenController can be disabled when a feature requires that the
  // toolbar be fully visible.  Since there are multiple reasons fullscreen
  // might need to be disabled, this state is represented by a counter rather
  // than a single bool.  When a feature needs the toolbar to be visible, it
  // calls IncrementDisabledCounter().  After that feature no longer requires
  // the toolbar, it calls DecrementDisabledCounter().  IsEnabled() returns
  // true when the counter is equal to zero.
  bool IsEnabled() const;
  void IncrementDisabledCounter();
  void DecrementDisabledCounter();

 private:
  friend class BrowserUserData<FullscreenController>;

  // Private constructor used by factory method.
  explicit FullscreenController(Browser* browser);

  // The model used to calculate fullscreen state.
  std::unique_ptr<FullscreenModel> model_;
  // The observer manager.
  std::unique_ptr<FullscreenControllerObserverManager> observer_manager_;
  // The reciever for fullscreen broadcast values.
  __strong id<FullscreenBroadcastReceiving> receiver_ = nil;
  // An object that forwards broadcasted UI values to |model_|.
  __strong FullscreenBroadcastForwarder* forwarder_ = nil;
  // A WebStateListObserver that updates |model_| for WebStateList changes.
  std::unique_ptr<FullscreenWebStateListObserver> web_state_list_observer_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenController);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_H_
