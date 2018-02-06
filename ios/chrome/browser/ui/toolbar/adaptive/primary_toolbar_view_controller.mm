// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_to_top_animator.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PrimaryToolbarViewController ()
// Redefined to be a PrimaryToolbarView.
@property(nonatomic, strong) PrimaryToolbarView* view;
@end

@implementation PrimaryToolbarViewController

@dynamic view;

#pragma mark - UIViewController

- (void)loadView {
  DCHECK(self.buttonFactory);
  self.view = [[PrimaryToolbarView alloc] init];
  self.view.buttonFactory = self.buttonFactory;
  if (@available(iOS 11, *)) {
    self.view.topSafeAnchor = self.view.safeAreaLayoutGuide.topAnchor;
  } else {
    self.view.topSafeAnchor = self.topLayoutGuide.bottomAnchor;
  }
}

#pragma mark - Property accessors

- (void)setLocationBarView:(UIView*)locationBarView {
  self.view.locationBarView = locationBarView;
}

#pragma mark - FullscreenUIElement

- (void)updateForFullscreenProgress:(CGFloat)progress {
  self.view.leadingStackView.alpha = progress;
  self.view.trailingStackView.alpha = progress;
  // TODO(crbug.com/804731): Update the location bar constraints.
}

- (void)updateForFullscreenEnabled:(BOOL)enabled {
  if (!enabled)
    [self updateForFullscreenProgress:1.0];
}

- (void)finishFullscreenScrollWithAnimator:
    (FullscreenScrollEndAnimator*)animator {
  [self addFullscreenAnimationsToAnimator:animator];
}

- (void)scrollFullscreenToTopWithAnimator:
    (FullscreenScrollToTopAnimator*)animator {
  [self addFullscreenAnimationsToAnimator:animator];
}

#pragma mark - FullscreenUIElement helpers

- (void)addFullscreenAnimationsToAnimator:(FullscreenAnimator*)animator {
  CGFloat finalProgress = animator.finalProgress;
  [animator addAnimations:^{
    [self updateForFullscreenProgress:finalProgress];
  }];
}

@end
