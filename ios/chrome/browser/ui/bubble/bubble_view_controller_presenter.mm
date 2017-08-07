// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_view_controller_presenter.h"

#include "base/logging.h"
#include "ios/chrome/browser/ui/bubble/bubble_util.h"
#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// How long, in seconds, the bubble is visible on the screen.
const NSTimeInterval kBubbleVisibilityDuration = 3.0;
// How long, in seconds, the user should be considered engaged with the bubble
// after the bubble first becomes visible.
const NSTimeInterval kBubbleEngagementDuration = 30.0;

}  // namespace

@interface BubbleViewControllerPresenter ()<UIGestureRecognizerDelegate>

// Redeclared as readwrite so the value can be changed internally.
@property(nonatomic, assign, readwrite, getter=isUserEngaged) BOOL userEngaged;
// The underlying BubbleViewController managed by this object.
// |bubbleViewController| manages the BubbleView instance.
@property(nonatomic, strong) BubbleViewController* bubbleViewController;
// The tap gesture recognizer intercepting tap gestures occurring inside the
// bubble view. Taps inside must be differentiated from taps outside to track
// UMA metrics. If users frequently tap inside the bubble (which does nothing
// but dismiss it, no different than tapping outside), it may indicate that
// users are not interacting with the bubble properly.
@property(nonatomic, strong) UITapGestureRecognizer* insideBubbleTapRecognizer;
// The tap gesture recognizer intercepting tap gestures occurring outside the
// bubble view. Does not prevent interactions with elements being tapped on.
// For example, tapping on a button both dismisses the bubble and triggers the
// button's action.
@property(nonatomic, strong) UITapGestureRecognizer* outsideBubbleTapRecognizer;
// The timer used to dismiss the bubble after a certain length of time. The
// bubble is dismissed automatically if the user does not dismiss it manually.
// If the user dismisses it manually, this timer is invalidated.
@property(nonatomic, strong) NSTimer* bubbleDismissalTimer;
// The timer used to reset the user's engagement. The user is considered
// engaged with the bubble while it is visible and for a certain duration after
// it disappears.
@property(nonatomic, strong) NSTimer* engagementTimer;
// The direction the underlying BubbleView's arrow is pointing.
@property(nonatomic, assign) BubbleArrowDirection arrowDirection;
// The alignment of the underlying BubbleView's arrow.
@property(nonatomic, assign) BubbleAlignment alignment;
// The block invoked when the bubble is dismissed (both via timer and via tap).
// Is optional.
@property(nonatomic, strong) ProceduralBlock dismissalCallback;

@end

@implementation BubbleViewControllerPresenter

@synthesize bubbleViewController = _bubbleViewController;
@synthesize insideBubbleTapRecognizer = _insideBubbleTapRecognizer;
@synthesize outsideBubbleTapRecognizer = _outsideBubbleTapRecognizer;
@synthesize bubbleDismissalTimer = _bubbleDismissalTimer;
@synthesize engagementTimer = _engagementTimer;
@synthesize userEngaged = _userEngaged;
@synthesize arrowDirection = _arrowDirection;
@synthesize alignment = _alignment;
@synthesize dismissalCallback = _dismissalCallback;

- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)arrowDirection
                   alignment:(BubbleAlignment)alignment
           dismissalCallback:(ProceduralBlock)dismissalCallback {
  self = [super init];
  if (self) {
    self.bubbleViewController =
        [[BubbleViewController alloc] initWithText:text
                                    arrowDirection:arrowDirection
                                         alignment:alignment];
    self.outsideBubbleTapRecognizer = [[UITapGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(tapOutsideBubbleRecognized:)];
    self.outsideBubbleTapRecognizer.delegate = self;
    self.outsideBubbleTapRecognizer.cancelsTouchesInView = NO;
    self.insideBubbleTapRecognizer = [[UITapGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(tapInsideBubbleRecognized:)];
    self.insideBubbleTapRecognizer.delegate = self;
    self.insideBubbleTapRecognizer.cancelsTouchesInView = NO;
    self.userEngaged = YES;
    self.arrowDirection = arrowDirection;
    self.alignment = alignment;
    self.dismissalCallback = dismissalCallback;
    // Don't initialize the timers yet because they should start when the bubble
    // first becomes visible, which is not necessarily at initialization of the
    // BubbleViewControllerPresenter.
  }
  return self;
}

- (void)tapInsideBubbleRecognized:(id)sender {
  [self dismissAnimated:YES];
}

- (void)tapOutsideBubbleRecognized:(id)sender {
  [self dismissAnimated:YES];
}

- (void)presentInViewController:(UIViewController*)parentViewController
                           view:(UIView*)parentView
                    anchorPoint:(CGPoint)anchorPoint {
  [parentViewController addChildViewController:self.bubbleViewController];

  CGPoint anchorPointInParent =
      [parentView.window convertPoint:anchorPoint toView:parentView];
  self.bubbleViewController.view.frame =
      [self frameForBubbleInRect:parentView.bounds
                   atAnchorPoint:anchorPointInParent];
  [parentView addSubview:self.bubbleViewController.view];
  [self.bubbleViewController animateContentIn];

  [self.bubbleViewController.view
      addGestureRecognizer:self.insideBubbleTapRecognizer];
  [parentView addGestureRecognizer:self.outsideBubbleTapRecognizer];

  self.bubbleDismissalTimer = [NSTimer
      scheduledTimerWithTimeInterval:kBubbleVisibilityDuration
                              target:self
                            selector:@selector(bubbleDismissalTimerFired:)
                            userInfo:nil
                             repeats:NO];
  self.engagementTimer =
      [NSTimer scheduledTimerWithTimeInterval:kBubbleEngagementDuration
                                       target:self
                                     selector:@selector(engagementTimerFired:)
                                     userInfo:nil
                                      repeats:NO];
}

- (void)dismissAnimated:(BOOL)animated {
  // Because this object must stay in memory to handle the |userEngaged|
  // property correctly, it is possible for |dismissAnimated| to be called
  // multiple times. However, only the first call should have any effect.
  if (self.bubbleViewController.parentViewController == nil) {
    return;
  }

  [self.bubbleDismissalTimer invalidate];
  self.bubbleDismissalTimer = nil;
  [self.insideBubbleTapRecognizer.view
      removeGestureRecognizer:self.insideBubbleTapRecognizer];
  [self.outsideBubbleTapRecognizer.view
      removeGestureRecognizer:self.outsideBubbleTapRecognizer];
  [self.bubbleViewController dismissAnimated:animated];
  [self.bubbleViewController willMoveToParentViewController:nil];
  [self.bubbleViewController removeFromParentViewController];

  if (self.dismissalCallback) {
    self.dismissalCallback();
  }
}

- (void)dealloc {
  [self.bubbleDismissalTimer invalidate];
  self.bubbleDismissalTimer = nil;
  [self.engagementTimer invalidate];
  self.engagementTimer = nil;
  [self.insideBubbleTapRecognizer.view
      removeGestureRecognizer:self.insideBubbleTapRecognizer];
  [self.outsideBubbleTapRecognizer.view
      removeGestureRecognizer:self.outsideBubbleTapRecognizer];
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  return gestureRecognizer != self.insideBubbleTapRecognizer &&
         otherGestureRecognizer != self.insideBubbleTapRecognizer;
}

#pragma mark - Private

// Automatically dismisses the bubble view when |bubbleDismissalTimer| fires.
- (void)bubbleDismissalTimerFired:(id)sender {
  [self dismissAnimated:YES];
}

// Marks the user as not engaged when |engagementTimer| fires.
- (void)engagementTimerFired:(id)sender {
  self.userEngaged = NO;
  self.engagementTimer = nil;
}

// Calculates the frame of the BubbleView. |rect| is the frame of the bubble's
// superview. |anchorPoint| is the anchor point of the bubble. |anchorPoint|
// and |rect| must be in the same coordinates.
- (CGRect)frameForBubbleInRect:(CGRect)rect atAnchorPoint:(CGPoint)anchorPoint {
  CGSize maxBubbleSize = bubble_util::BubbleMaxSize(
      anchorPoint, self.arrowDirection, self.alignment, rect.size);
  CGSize bubbleSize =
      [self.bubbleViewController.view sizeThatFits:maxBubbleSize];
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize, self.arrowDirection,
                               self.alignment, CGRectGetWidth(rect));
  return bubbleFrame;
}

@end
