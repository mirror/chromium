// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_controller.h"

#import <Foundation/Foundation.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_broadcast_forwarder.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_controller_observer.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_web_state_list_observer.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(FullscreenController);

FullscreenController::FullscreenController(Browser* browser)
    : model_(base::MakeUnique<FullscreenModel>(this)),
      forwarder_([[FullscreenBroadcastForwarder alloc]
          initWithBroadcaster:browser->broadcaster()
                        model:model_.get()]),
      web_state_list_observer_(base::MakeUnique<FullscreenWebStateListObserver>(
          model_.get(),
          &browser->web_state_list())) {}

FullscreenController::~FullscreenController() {
  [forwarder_ disconnect];
}

bool FullscreenController::IsDisabled() const {
  return model_->disabled();
}

void FullscreenController::IncrementDisabledCounter() {
  model_->IncrementDisabledCounter();
}

void FullscreenController::DecrementDisabledCounter() {
  model_->DecrementDisabledCounter();
}

void FullscreenController::OnProgressUpdated() {
  CleanUpAnimations();
  for (auto& observer : observers_) {
    observer.FullscreenProgressUpdated(this, model_->progress());
  }
}

void FullscreenController::OnFullscreenDisabled() {
  CleanUpAnimations();
  for (auto& observer : observers_) {
    observer.FullscreenDisabled(this);
  }
}

void FullscreenController::OnFullscreenEnabled() {
  CleanUpAnimations();
  for (auto& observer : observers_) {
    observer.FullscreenEnabled(this);
  }
}

void FullscreenController::OnScrollEventEnded() {
  DCHECK(!animator_);
  animator_ = [[FullscreenScrollEndAnimator alloc]
      initWithStartProgress:model_->progress()];
  [animator_ addCompletion:^(UIViewAnimatingPosition finalPosition) {
    CleanUpAnimations();
  }];
  for (auto& observer : observers_) {
    observer.FullscreenScrollEventEnded(this, animator_);
  }
  [animator_ startAnimation];
}

void FullscreenController::CleanUpAnimations() {
  if (!animator_)
    return;
  model_->AnimationEndedWithProgress(animator_.currentProgress);
  [animator_ stopAnimation:YES];
  animator_ = nil;
}
