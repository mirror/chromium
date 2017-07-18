// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/bubble/sc_bubble_coordinator.h"

#import "ios/chrome/browser/ui/bubble/bubble_util.h"
#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SCBubbleCoordinator
@synthesize baseViewController = _baseViewController;

- (void)start {
  UIViewController* containerViewController = [[UIViewController alloc] init];
  UIView* containerView = containerViewController.view;
  containerView.backgroundColor = [UIColor whiteColor];
  containerViewController.title = @"Bubble";

  // Initialize BubbleVC with upwards arrow direction, trailing alignment, and
  // example display text.
  BubbleArrowDirection direction = BubbleArrowDirectionUp;
  BubbleAlignment alignment = BubbleAlignmentTrailing;
  BubbleViewController* bubbleViewController =
      [[BubbleViewController alloc] initWithText:@"Lorem ipsum dolor"
                                  arrowDirection:direction
                                       alignment:alignment];
  [containerViewController addChildViewController:bubbleViewController];

  // Mock UI element for the bubble to be anchored on.
  UIView* elementView = [[UIView alloc]
      initWithFrame:CGRectMake(CGRectGetMaxX(containerView.frame) - 20.0f, 0.0f,
                               20.0f, 20.0f)];
  elementView.backgroundColor = [UIColor grayColor];
  [containerView addSubview:elementView];
  CGSize maxSize = CGSizeMake(
      containerView.frame.size.width,
      containerView.frame.size.height - CGRectGetMaxY(elementView.frame));
  // Since the mock UI element is positioned at the top-right corner, set the
  // navigation translucency to NO so it doesn't cover the element.
  self.baseViewController.navigationBar.translucent = NO;

  // Set |bubbleViewController.view.frame| using bubbleView's |sizeThatFits| and
  // bubble_util methods.
  CGSize bubbleSize = [bubbleViewController.view sizeThatFits:maxSize];
  CGPoint anchorPoint = BubbleAnchorPoint(elementView.frame, direction);
  CGPoint origin = BubbleOrigin(anchorPoint, alignment, direction, bubbleSize);
  bubbleViewController.view.frame =
      CGRectMake(origin.x, origin.y, bubbleSize.width, bubbleSize.height);
  bubbleViewController.view.backgroundColor = [UIColor blueColor];
  [containerView addSubview:bubbleViewController.view];
  [bubbleViewController didMoveToParentViewController:containerViewController];

  [self.baseViewController pushViewController:containerViewController
                                     animated:YES];
}

@end
