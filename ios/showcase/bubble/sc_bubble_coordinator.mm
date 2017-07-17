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
  containerViewController.view.backgroundColor = [UIColor whiteColor];
  containerViewController.title = @"Bubble";

  // Initialize BubbleVC with upwards arrow direction and example display text.
  BubbleArrowDirection direction = BubbleArrowDirectionUp;
  BubbleViewController* bubbleViewController =
      [[BubbleViewController alloc] initWithText:@"Lorem ipsum dolor"
                                  arrowDirection:direction];
  [containerViewController addChildViewController:bubbleViewController];

  // Mock UI element for the bubble to be anchored on.
  UIView* elementView =
      [[UIView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 20.0f, 20.0f)];
  elementView.center = containerViewController.view.center;
  elementView.backgroundColor = [UIColor grayColor];
  [containerViewController.view addSubview:elementView];

  // Set |bubbleViewController.view.frame| using bubbleView's |sizeThatFits| and
  // bubble_util methods.
  CGSize bubbleSize = [bubbleViewController.view
      sizeThatFits:containerViewController.view.frame.size];
  CGPoint anchorPoint = BubbleAnchorPoint(elementView.frame, direction);
  CGPoint origin = BubbleOrigin(anchorPoint, direction, bubbleSize,
                                containerViewController.view.frame);
  bubbleViewController.view.frame.origin = origin;
  bubbleViewController.view.frame.size = bubbleSize;
  // Convert |anchorPoint| to |bubbleViewController.view|'s coordinate system.
  CGPoint anchorPointForBubble =
      [containerViewController.view convertPoint:anchorPoint
                                          toView:bubbleViewController.view];
  [bubbleViewController anchorOnPoint:anchorPointForBubble];
  [containerViewController.view addSubview:bubbleViewController.view];
  [bubbleViewController didMoveToParentViewController:containerViewController];

  [self.baseViewController pushViewController:containerViewController
                                     animated:YES];
}

@end
