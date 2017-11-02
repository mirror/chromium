// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer_manager.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenControllerObserverManager::FullscreenControllerObserverManager(
    FullscreenController* controller,
    FullscreenModel* model)
    : controller_(controller), model_(model), animation_controller_(model_) {
  DCHECK(controller_);
  DCHECK(model_);
  model_->AddObserver(this);
}

FullscreenControllerObserverManager::~FullscreenControllerObserverManager() {
  // Disconnect() is expected to be called before deallocation.
  DCHECK(!controller_);
  DCHECK(!model_);
}

void FullscreenControllerObserverManager::Disconnect() {
  animation_controller_.StopAnimating();
  model_->RemoveObserver(this);
  model_ = nullptr;
  controller_ = nullptr;
}

void FullscreenControllerObserverManager::FullscreenModelProgressUpdated(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  animation_controller_.StopAnimating();
  for (auto& observer : observers_) {
    observer.FullscreenProgressUpdated(controller_, model_->progress());
  }
}

void FullscreenControllerObserverManager::FullscreenModelEnabledStateChanged(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  animation_controller_.StopAnimating();
  for (auto& observer : observers_) {
    observer.FullscreenEnabledStateChanged(controller_, model->enabled());
  }
}

void FullscreenControllerObserverManager::FullscreenModelScrollEventEnded(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  animation_controller_.CreateAnimator(model_->progress());
  FullscreenScrollEndAnimator* animator = animation_controller_.animator();
  for (auto& observer : observers_) {
    observer.FullscreenScrollEventEnded(controller_, animator);
  }
#if defined(__IPHONE_10_0) && (__IPHONE_OS_VERSION_MIN_ALLOWED >= __IPHONE_10_0)
  // TODO(crbug.com/778858): Remove these ifdefs once UIViewPropertyAnimators
  // are supported.
  [animator startAnimation];
#endif  // __IPHONE_10_0
}
