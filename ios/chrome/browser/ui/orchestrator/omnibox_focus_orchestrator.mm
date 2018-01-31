// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/orchestrator/omnibox_focus_orchestrator.h"

#import "ios/chrome/browser/ui/orchestrator/toolbar_animatee.h"
#import "ios/chrome/common/material_timing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OmniboxFocusOrchestrator

@synthesize toolbarAnimatee = _toolbarAnimatee;

- (void)expandOmnibox:(BOOL)animated {
  void (^expansion)() = ^{
    [self.toolbarAnimatee expandLocationBar];
    [self.toolbarAnimatee showCancelButton];
  };

  void (^hideControls)() = ^{
    [self.toolbarAnimatee hideControlButtons];
  };

  if (animated) {
    UIViewPropertyAnimator* slowAnimator = [[UIViewPropertyAnimator alloc]
        initWithDuration:ios::material::kDuration2
                   curve:UIViewAnimationCurveEaseInOut
              animations:^{
                expansion();
              }];

    UIViewPropertyAnimator* fastAnimator = [[UIViewPropertyAnimator alloc]
        initWithDuration:ios::material::kDuration1
                   curve:UIViewAnimationCurveEaseInOut
              animations:^{
                hideControls();
              }];

    [slowAnimator startAnimation];
    [fastAnimator startAnimation];
  } else {
    expansion();
    hideControls();
  }
}

- (void)contractOmnibox:(BOOL)animated {
}

@end
