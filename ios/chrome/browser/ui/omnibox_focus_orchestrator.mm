// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox_focus_orchestrator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OmniboxFocusOrchestrator
@synthesize toolbarAnimatee;
@synthesize locationBarAnimatee;
@synthesize omniboxAnimatee;

- (void)expandOmnibox:(BOOL)animated {
  [self.toolbarAnimatee prepareToExpand];
  [self.locationBarAnimatee prepareToExpand];
  [self.omniboxAnimatee prepareToExpand];

  UIViewPropertyAnimator* phase1_slow = [[UIViewPropertyAnimator alloc]
      initWithDuration:ios::material::kDuration1
                 curve:UIViewAnimationCurveEaseInOut
            animations:^{
              [self.toolbarAnimatee expandLocationBarContainer];
              [self.toolbarAnimatee showIncognitoButtonIfNecessary];
              [self.locationBarAnimatee hideLeadingButtonIfNecessary];
            }];

  UIViewPropertyAnimator* phase1_fast = [[UIViewPropertyAnimator alloc]
      initWithDuration:ios::material::kDuration2
                 curve:UIViewAnimationOptionCurveEaseIn
            animations:^{
              [self.toolbarAnimatee hideButtons];
            }];

  UIViewPropertyAnimator* phase2 = [[UIViewPropertyAnimator alloc]
      initWithDuration:ios::material::kDuration1
                 curve:UIViewAnimationOptionCurveEaseOut
            animations:^{
              [self.toolbarAnimatee showContractButton];
              [self.omniboxAnimatee showClearButton];
            }];

  [phase1_slow addCompletion:^(UIViewAnimatingPosition finalPosition) {
    [self.toolbarAnimatee hideBorder];
    [phase2 startAnimationAfterDelay:ios::material::kDuration4];
  }];

  [phase1_slow startAnimation];
  [phase1_fast startAnimation];

  if (!animated) {
    [phase1_slow stopAnimation:NO];
    [phase1_slow finishAnimationAtPosition:UIViewAnimatingPositionEnd];
    [phase1_fast stopAnimation:NO];
    [phase1_fast finishAnimationAtPosition:UIViewAnimatingPositionEnd];
  }
}

- (void)contractOmnibox:(BOOL)animated {
  // Do the same for contract omnibox.
}

@end
