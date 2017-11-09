// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/view_info.h"

#include "base/memory/ptr_util.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for ViewInfos.
class ViewInfoTest : public PlatformTest {
 public:
  ViewInfoTest()
      : PlatformTest(),
        parent_([[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)]),
        view_([[UIView alloc] initWithFrame:parent_.bounds]),
        info_(view_) {
    [parent_ addSubview:view_];
  }

  UIView* parent() { return parent_; }
  UIView* view() { return view_; }
  const ViewInfo& info() { return info_; }

 private:
  __strong UIView* parent_ = nullptr;
  __strong UIView* view_ = nullptr;
  ViewInfo info_;
};

// Tests that IsViewAlive returns false when there is no view.
TEST_F(ViewInfoTest, ViewNotAlive) {
  ViewInfo info(nil);
  EXPECT_FALSE(info.alive());
}

// Tests that frame() returns the view's frame.
TEST_F(ViewInfoTest, Frame) {
  EXPECT_TRUE(CGRectEqualToRect(info().frame(), parent().bounds));
  const CGRect kNewFrame = CGRectMake(1, 2, 3, 4);
  EXPECT_TRUE(CGRectEqualToRect(info().frame(), kNewFrame));
}

// Tests that center() returns the view's center.
TEST_F(ViewInfoTest, Center) {
  EXPECT_TRUE(CGPointEqualToPoint(info().frame(), CGPointMake(50, 50)));
  view().frame = CGRectMake(50, 50, 50, 50);
  EXPECT_TRUE(CGPointEqualToPoint(info().frame(), CGPointMake(75, 75)));
}

// Tests that GetFrame() returns the view's frame in the parent's coordinate
// system.
TEST_F(ViewInfoTest, GetFrameInView) {
  EXPECT_TRUE(
      CGRectEqualToRect(info().GetFrameInView(parent()), parent().bounds));
  view().frame = CGRectMake(50, 50, 50, 50);
  EXPECT_TRUE(CGRectEqualToRect(info().GetFrameInView(parent()),
                                CGRectMake(50, 50, 50, 50)));
}

// Tests that GetFrame returns the view's frame in window coordinates when it's
// a subview.
TEST_F(ViewInfoTest, GetFrameSubview) {
  view().frame = CGRectMake(50, 50, 50, 50);
  UIView* subview = [[UIView alloc] initWithFrame:CGRectMake(25, 25, 25, 25)];
  [view() addSubview:subview];
  ViewInfo subview_info(subview);
  EXPECT_TRUE(CGRectEqualToRect(subview_info.GetFrameInView(parent()),
                                CGRectMake(75, 75, 25, 25)));
}

// Tests that GetCenter() returns the view's center.
TEST_F(ViewInfoTest, GetCenter) {
  EXPECT_TRUE(
      CGPointEqualToPoint(info().GetCenterInView(parent()), parent().center));
  view().frame = CGRectMake(50, 50, 50, 50);
  EXPECT_TRUE(CGPointEqualToPoint(info().GetCenterInView(parent()),
                                  CGPointMake(75, 75)));
}

// Tests that GetFrame returns the view's frame in window coordinates when it's
// a subview.
TEST_F(ViewInfoTest, GetCenterSubview) {
  view().frame = CGRectMake(50, 50, 50, 50);
  UIView* subview = [[UIView alloc] initWithFrame:CGRectMake(25, 25, 25, 25)];
  [view() addSubview:subview];
  ViewInfo subview_info(subview);
  EXPECT_TRUE(CGPointEqualToPoint(subview_info.GetCenterInView(parent()),
                                  CGPointMake(87.5, 87.5)));
}
