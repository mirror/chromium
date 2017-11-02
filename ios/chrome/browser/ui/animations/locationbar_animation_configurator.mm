// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/animations/locationbar_animation_configurator.h"

#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_constants.h"
#import "ios/chrome/browser/ui/toolbar/public/web_toolbar_controller_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LocationbarAnimationConfigurator ()

// UIViewPropertyAnimator for animating the location bar.
@property(nonatomic, strong, readwrite)
    UIViewPropertyAnimator* locationbarAnimator API_AVAILABLE(ios(10.0));

@end

@implementation LocationbarAnimationConfigurator

@synthesize locationbarAnimator = _locationbarAnimator;
@synthesize expanded = _expanded;
@synthesize fadingLeadingButtons = _fadingLeadingButtons;
@synthesize fadingTrailingButtons = _fadingTrailingButtons;
@synthesize fadingLeadingViews = _fadingLeadingViews;
@synthesize fadingTrailingViews = _fadingTrailingViews;
@synthesize animatingBackground = _animatingBackground;
@synthesize animatingLocationBar = _animatingLocationBar;
@synthesize finalBackgroundFrame = _finalBackgroundFrame;
@synthesize finalLocationBarFrame = _finalLocationBarFrame;
@synthesize incognitoIcon = _incognitoIcon;

- (instancetype)init {
  self = [super init];
  if (self) {
    _expanded = NO;
  }
  return self;
}

- (void)runAnimation {
  if (@available(iOS 10, *)) {
    self.locationbarAnimator = [[UIViewPropertyAnimator alloc]
        initWithDuration:0.20
                   curve:UIViewAnimationCurveEaseInOut
              animations:^{
              }];
    if (self.isExpanded) {
      [self contractOmnibox];
    } else {
      [self expandOmnibox];
    }
  }
}

- (void)expandOmnibox API_AVAILABLE(ios(10.0)) {
  // Return if PropertyAnimator is already running.
  if (self.locationbarAnimator.isRunning)
    return;

  __weak UIView* weakAnimatingLocationBar = self.animatingLocationBar;
  __weak UIView* weakAnimatingBackground = self.animatingBackground;
  CGRect weakExpandedLocationBar = self.finalLocationBarFrame;
  CGRect weakExpandedBackground = self.finalBackgroundFrame;
  // Create animator and add animations.
  [self.locationbarAnimator addAnimations:^{
    // Omnibox and OmniboxBackground.
    weakAnimatingLocationBar.frame = weakExpandedLocationBar;
    weakAnimatingBackground.frame = weakExpandedBackground;
  }];
  // Add standard Toolbar buttons animations.
  [self configureExpandAnimation];

  [self.locationbarAnimator startAnimation];
  // Change this on PropertyAnimator Completion
  self.expanded = YES;
}

- (void)contractOmnibox API_AVAILABLE(ios(10.0)) {
  // Return if PropertyAnimator is already running.
  if (self.locationbarAnimator.isRunning)
    return;

  __weak UIView* weakAnimatingLocationBar = self.animatingLocationBar;
  __weak UIView* weakAnimatingBackground = self.animatingBackground;
  CGRect weakExpandedLocationBar = self.finalLocationBarFrame;
  CGRect weakExpandedBackground = self.finalBackgroundFrame;

  [self.locationbarAnimator addAnimations:^{

    // Omnibox and OmniboxBackground.
    weakAnimatingLocationBar.frame = weakExpandedLocationBar;
    weakAnimatingBackground.frame = weakExpandedBackground;

  }];

  [self configureContractAnimation];

  [self.locationbarAnimator startAnimation];
  // Change this on PropertyAnimator Completion
  self.expanded = NO;
}

- (void)configureExpandAnimation API_AVAILABLE(ios(10.0)) {
  //  __weak UIView* weakShadowView = shadowView_;
  //  __weak UIView* weakFullBleedShadowView = fullBleedShadowView_;
  // Fade to the full bleed shadow.
  //    weakShadowView.alpha = 0;
  //    weakFullBleedShadowView.alpha = 1;

  [self fadeOutViews:self.fadingTrailingButtons
          withOffset:kButtonFadeOutXOffset];
  [self fadeOutViews:self.fadingLeadingButtons
          withOffset:-kButtonFadeOutXOffset];

  // Leading Views.
  NSArray* leadingViews = self.fadingLeadingViews;
  [self.locationbarAnimator addAnimations:^{
    for (UIView* leadingView in leadingViews) {
      leadingView.alpha = 0;
      leadingView.frame = CGRectLayoutOffset(leadingView.frame,
                                             -kPositionAnimationLeadingOffset);
    }
  }];

  // Trailing Views.
  NSArray* trailingViews = self.fadingTrailingViews;

  // Make sure the Alpha is 0 for the trailingViews.
  for (UIView* trailingView in trailingViews) {
    trailingView.alpha = 0;
  }

  [self.locationbarAnimator addCompletion:^(
                                UIViewAnimatingPosition finalPosition) {
    for (UIView* trailingView in trailingViews) {
      trailingView.frame = CGRectLayoutOffset(trailingView.frame,
                                              kPositionAnimationLeadingOffset);
    }
    [UIViewPropertyAnimator
        runningPropertyAnimatorWithDuration:0.2
                                      delay:0.1
                                    options:UIViewAnimationOptionCurveEaseOut
                                 animations:^{
                                   for (UIView* trailingView in trailingViews) {
                                     trailingView.alpha = 1.0;
                                     trailingView.frame = CGRectLayoutOffset(
                                         trailingView.frame,
                                         -kPositionAnimationLeadingOffset);
                                   }
                                 }
                                 completion:nil];
  }];

  // Incognito Icon
  if (self.incognitoIcon) {
    [self fadeInViews:@[ self.incognitoIcon ]
           withOffset:kPositionAnimationLeadingOffset];
  }
}

- (void)configureContractAnimation API_AVAILABLE(ios(10.0)) {
  [self fadeInViews:self.fadingTrailingButtons
         withOffset:kButtonFadeOutXOffset];
  [self fadeInViews:self.fadingLeadingButtons
         withOffset:-kButtonFadeOutXOffset];

  // Leading Views.
  for (UIView* leadingView in self.fadingLeadingViews) {
    leadingView.alpha = 0;
    leadingView.frame =
        CGRectLayoutOffset(leadingView.frame, kPositionAnimationLeadingOffset);
  }

  NSArray* leadingViews = self.fadingLeadingViews;
  [self.locationbarAnimator addAnimations:^{
    for (UIView* leadingView in leadingViews) {
      leadingView.alpha = 1;
      leadingView.frame = CGRectLayoutOffset(leadingView.frame,
                                             -kPositionAnimationLeadingOffset);
    }
  }];

  // Trailing Views.
  NSArray* trailingViews = self.fadingTrailingViews;
  [self.locationbarAnimator addAnimations:^{
    for (UIView* trailingView in trailingViews) {
      trailingView.alpha = 0;
      trailingView.frame = CGRectLayoutOffset(trailingView.frame,
                                              +kPositionAnimationLeadingOffset);
    }
  }];

  // Incognito Icon
  if (self.incognitoIcon) {
    [self fadeOutViews:@[ self.incognitoIcon ]
            withOffset:-kPositionAnimationLeadingOffset];
  }
}

#pragma mark - Helper Functions.

- (void)fadeInViews:(NSArray*)views
         withOffset:(LayoutOffset)offset API_AVAILABLE(ios(10.0)) {
  // TODO Button fade in needs to happen shortly after the omnibox has been
  // contracted.

  // Shift the buttons in preparation for the animation. (Related to bug)
  for (UIView* view in views) {
    view.frame = CGRectLayoutOffset(view.frame, offset);
  }

  [self.locationbarAnimator addAnimations:^{
    for (UIView* view in views) {
      if (![view isHidden]) {
        view.alpha = 1;
        view.frame = CGRectOffset(view.frame, -offset, 0);
      }
    }
  }];
}

- (void)fadeOutViews:(NSArray*)views
          withOffset:(LayoutOffset)offset API_AVAILABLE(ios(10.0)) {
  [self.locationbarAnimator addAnimations:^{
    for (UIView* view in views) {
      if (![view isHidden]) {
        view.alpha = 0;
        view.frame = CGRectOffset(view.frame, offset, 0);
      }
    }
  }];

  // After the animation is done and the buttons are hidden, move the buttons
  // back to the position they originally were.
  // Bug. This is needed because the button position is used to calculate the
  // contracting omnibox size, so if we leave the buttons shifted it will
  // wrongly calculate the new Omnibox size.
  [self.locationbarAnimator
      addCompletion:^(UIViewAnimatingPosition finalPosition) {
        for (UIView* view in views) {
          view.frame = CGRectOffset(view.frame, -offset, 0);
        }
      }];
}

#pragma mark - Getters.

- (NSMutableArray*)fadingLeadingButtons {
  if (!_fadingLeadingButtons) {
    _fadingLeadingButtons = [NSMutableArray array];
  }
  return _fadingLeadingButtons;
}

- (NSMutableArray*)fadingTrailingButtons {
  if (!_fadingTrailingButtons) {
    _fadingTrailingButtons = [NSMutableArray array];
  }
  return _fadingTrailingButtons;
}

- (NSMutableArray*)fadingLeadingViews {
  if (!_fadingLeadingViews) {
    _fadingLeadingViews = [NSMutableArray array];
  }
  return _fadingLeadingViews;
}

- (NSMutableArray*)fadingTrailingViews {
  if (!_fadingTrailingViews) {
    _fadingTrailingViews = [NSMutableArray array];
  }
  return _fadingTrailingViews;
}

@end
