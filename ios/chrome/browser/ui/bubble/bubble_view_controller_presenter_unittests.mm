// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/bubble/bubble_view.h"
#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"
#import "ios/chrome/browser/ui/bubble/bubble_view_controller_presenter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BubbleViewControllerPresenter (Testing)

// Underlying bubble view controller managed by the presenter.
@property(nonatomic, strong) BubbleViewController* bubbleViewController;

@end

// Test fixture to test the BubbleViewControllerPresenter.
class BubbleViewControllerPresenterTest : public PlatformTest {
 public:
  BubbleViewControllerPresenterTest()
      : bubbleViewControllerPresenter_([[BubbleViewControllerPresenter alloc]
                 initWithText:@"Text"
               arrowDirection:BubbleArrowDirectionUp
                    alignment:BubbleAlignmentCenter
            dismissalCallback:^() {
              dismissalCallbackCount_++;
            }]),
        parentViewController_([[UIViewController alloc] init]),
        dismissalCallbackCount_(0) {}

 protected:
  BubbleViewControllerPresenter* bubbleViewControllerPresenter_;
  UIViewController* parentViewController_;
  int dismissalCallbackCount_;
};

// Tests that after initialization the internal BubbleViewController and
// BubbleView have not been added to the parent.
TEST_F(BubbleViewControllerPresenterTest, InitializedNotAdded) {
  EXPECT_FALSE([parentViewController_.childViewControllers
      containsObject:bubbleViewControllerPresenter_.bubbleViewController]);
  EXPECT_FALSE([parentViewController_.view.subviews
      containsObject:bubbleViewControllerPresenter_.bubbleViewController.view]);
}

// Tests that -presentInViewController: adds the BubbleViewController and
// BubbleView to the parent.
TEST_F(BubbleViewControllerPresenterTest, PresentAddsToViewController) {
  [bubbleViewControllerPresenter_
      presentInViewController:parentViewController_
                         view:parentViewController_.view
                  anchorPoint:CGPointMake(0.0, 0.0)];
  EXPECT_TRUE([parentViewController_.childViewControllers
      containsObject:bubbleViewControllerPresenter_.bubbleViewController]);
  EXPECT_TRUE([parentViewController_.view.subviews
      containsObject:bubbleViewControllerPresenter_.bubbleViewController.view]);
}

// Tests that initially the dismissal callback has not been invoked.
TEST_F(BubbleViewControllerPresenterTest, DismissalCallbackCountInitialized) {
  EXPECT_EQ(0, dismissalCallbackCount_);
}

// Tests that presenting the bubble but not dismissing it does not invoke the
// dismissal callback.
TEST_F(BubbleViewControllerPresenterTest, DismissalCallbackNotCalled) {
  [bubbleViewControllerPresenter_
      presentInViewController:parentViewController_
                         view:parentViewController_.view
                  anchorPoint:CGPointMake(0.0, 0.0)];
  EXPECT_EQ(0, dismissalCallbackCount_);
}

// Tests that presenting then dismissing the bubble invokes the dismissal
// callback.
TEST_F(BubbleViewControllerPresenterTest, DismissalCallbackCalledOnce) {
  [bubbleViewControllerPresenter_
      presentInViewController:parentViewController_
                         view:parentViewController_.view
                  anchorPoint:CGPointMake(0.0, 0.0)];
  [bubbleViewControllerPresenter_ dismissAnimated:NO];
  EXPECT_EQ(1, dismissalCallbackCount_);
}

// Tests that calling -dismissAnimated: after the bubble has already been
// dismissed does not invoke the dismissal callback again.
TEST_F(BubbleViewControllerPresenterTest, DismissalCallbackNotCalledTwice) {
  [bubbleViewControllerPresenter_
      presentInViewController:parentViewController_
                         view:parentViewController_.view
                  anchorPoint:CGPointMake(0.0, 0.0)];
  [bubbleViewControllerPresenter_ dismissAnimated:NO];
  [bubbleViewControllerPresenter_ dismissAnimated:NO];
  EXPECT_EQ(1, dismissalCallbackCount_);
}

// Tests that calling -dismissAnimated: before the bubble has been presented
// does not invoke the dismissal callback.
TEST_F(BubbleViewControllerPresenterTest,
       DismissalCallbackNotCalledBeforePresentation) {
  [bubbleViewControllerPresenter_ dismissAnimated:NO];
  EXPECT_EQ(0, dismissalCallbackCount_);
}
