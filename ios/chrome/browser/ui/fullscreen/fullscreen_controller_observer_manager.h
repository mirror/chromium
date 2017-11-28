// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_OBSERVER_MANAGER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_OBSERVER_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model_observer.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animation_controller.h"

class FullscreenController;
class FullscreenControllerObserver;

// A helper object that listens to FullscreenModel changes and forwards this
// information to FullscreenControllerObservers.
class FullscreenControllerObserverManager : public FullscreenModelObserver {
 public:
  FullscreenControllerObserverManager(FullscreenController* controller,
                                      FullscreenModel* model);
  ~FullscreenControllerObserverManager() override;

  // Adds and removes FullscreenControllerObservers.
  void AddObserver(FullscreenControllerObserver* observer) {
    observers_.AddObserver(observer);
  }
  void RemoveObserver(FullscreenControllerObserver* observer) {
    observers_.RemoveObserver(observer);
  }

  // Instructs the manager to stop observing its model.
  void Disconnect();

 private:
  // FullscreenModelObserver:
  void FullscreenModelProgressUpdated(FullscreenModel* model) override;
  void FullscreenModelEnabledStateChanged(FullscreenModel* model) override;
  void FullscreenModelScrollEventEnded(FullscreenModel* model) override;

  // The controller.
  FullscreenController* controller_ = nullptr;
  // The model.
  FullscreenModel* model_ = nullptr;
  // The FullscreenControllerObservers that need to get notified of model
  // changes.
  base::ObserverList<FullscreenControllerObserver> observers_;
  // The animation controller to use for scroll end animations.
  FullscreenScrollEndAnimationController animation_controller_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenControllerObserverManager);
};

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_OBSERVER_MANAGER_H_
