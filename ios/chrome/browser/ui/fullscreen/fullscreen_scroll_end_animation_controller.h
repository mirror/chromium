// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_SCROLL_END_ANIMATION_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_SCROLL_END_ANIMATION_CONTROLLER_H_

#import <CoreGraphics/CoreGraphics.h>

class FullscreenModel;
@class FullscreenScrollEndAnimator;

// An object that manages the fullscreen scroll end toolbar adjustment
// animation.
class FullscreenScrollEndAnimationController {
 public:
  explicit FullscreenScrollEndAnimationController(FullscreenModel* model);
  virtual ~FullscreenScrollEndAnimationController();

  // The animator for the scroll end adjustment.
  FullscreenScrollEndAnimator* animator() { return animator_; }

  // Creates a new FullscreenScrollEndAnimator with |start_progress|.  After
  // calling this function, animator() will return the newly-created animator.
  // In-progress animations are expected to be stopped via StopAnimating()
  // before this function is called.
  void CreateAnimator(CGFloat start_progress);

  // Stops any in-progress animations and updates the model for the current UI
  // state.
  void StopAnimating();

 private:
  // The model.
  FullscreenModel* model_ = nullptr;
  // The animator that is managed by this controller.
  __strong FullscreenScrollEndAnimator* animator_ = nil;
};

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_SCROLL_END_ANIMATION_CONTROLLER_H_
