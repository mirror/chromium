// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animation_controller.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenScrollEndAnimationController::FullscreenScrollEndAnimationController(
    FullscreenModel* model)
    : model_(model) {
  DCHECK(model_);
}

FullscreenScrollEndAnimationController::
    ~FullscreenScrollEndAnimationController() = default;

void FullscreenScrollEndAnimationController::CreateAnimator(
    CGFloat start_progress) {
#if defined(__IPHONE_10_0) && (__IPHONE_OS_VERSION_MIN_ALLOWED >= __IPHONE_10_0)
  // TODO(crbug.com/778858): Remove ifdefs once iOS9 support is dropped.
  DCHECK(!animator_);
  animator_ = [[FullscreenScrollEndAnimator alloc]
      initWithStartProgress:start_progress];
  [animator_ addCompletion:^(UIViewAnimatingPosition finalPosition) {
    StopAnimating();
  }];
#else
  animator_ = [[FullscreenScrollEndAnimator alloc] init];
#endif  // __IPHONE_10_0
}

void FullscreenScrollEndAnimationController::StopAnimating() {
  if (!animator_)
    return;

#if defined(__IPHONE_10_0) && (__IPHONE_OS_VERSION_MIN_ALLOWED >= __IPHONE_10_0)
  // TODO(crbug.com/778858): Remove ifdefs once iOS9 support is dropped.
  model_->AnimationEndedWithProgress(animator_.currentProgress);
  [animator_ stopAnimation:YES];
#endif  // __IPHONE_10_0
  animator_ = nil;
}
