// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/bubble/sc_bubble_coordinator.h"

#include "ios/chrome/browser/ui/bubble/bubble_util.h"
#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using namespace bubble_util;

@implementation SCBubbleCoordinator
@synthesize baseViewController = _baseViewController;

- (void)start {
  UIViewController* containerViewController = [[UIViewController alloc] init];
  UIView* containerView = containerViewController.view;
  containerView.backgroundColor = [UIColor whiteColor];
  containerViewController.title = @"Bubble";
  // Since the mock UI element is positioned at the top-right corner, set the
  // navigation translucency to NO so it doesn't cover the element.
  self.baseViewController.navigationBar.translucent = NO;

  // Initialize a |BubbleViewController| with upwards arrow direction, trailing
  // alignment, and example display text.
  BubbleArrowDirection direction = BubbleArrowDirectionUp;
  BubbleAlignment alignment = BubbleAlignmentTrailing;
  BubbleViewController* bubbleViewController =
      [[BubbleViewController alloc] initWithText:@"Lorem ipsum dolor"
                                  arrowDirection:direction
                                       alignment:alignment];

  // Mock UI element for the bubble to be anchored on.
  UIView* elementView = [[UIView alloc]
      initWithFrame:CGRectMake(containerView.frame.size.width - 40.0f, 20.0f,
                               20.0f, 20.0f)];
  elementView.backgroundColor = [UIColor grayColor];
  [containerView addSubview:elementView];
  // Maximum width of the bubble such that it stays within |containerView|.
  CGFloat maxBubbleWidth =
      MaxWidth(elementView.frame, alignment, containerView.frame.size.width);
  CGSize maxSize =
      CGSizeMake(maxBubbleWidth, containerView.frame.size.height -
                                     CGRectGetMaxY(elementView.frame));
  // Calculate the bubble's frame using |sizeThatFits| and bubble_util methods.
  CGSize bubbleSize = [bubbleViewController.view sizeThatFits:maxSize];
  CGPoint origin = Origin(elementView.frame, alignment, direction, bubbleSize);
  CGRect bubbleFrame =
      CGRectMake(origin.x, origin.y, bubbleSize.width, bubbleSize.height);

  // Add the |BubbleViewController| as a child view controller of the container.
  [containerViewController addChildViewController:bubbleViewController];
  bubbleViewController.view.frame = bubbleFrame;
  bubbleViewController.view.backgroundColor = [UIColor blueColor];
  [containerView addSubview:bubbleViewController.view];
  [bubbleViewController didMoveToParentViewController:containerViewController];

  [self.baseViewController pushViewController:containerViewController
                                     animated:YES];
}

@end
