// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/touch_action_region.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(TouchActionRegionTest, GetWhiteListedTouchActionMapOverlapToZero) {
  TouchAction touch_action;
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionPanLeft, gfx::Rect(0, 0, 50, 50));
  touch_action_region.Union(kTouchActionPanRight, gfx::Rect(25, 25, 25, 25));
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10));
  EXPECT_EQ(kTouchActionPanLeft, touch_action);
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(30, 30));
  EXPECT_EQ(kTouchActionNone, touch_action);
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(60, 60));
  EXPECT_EQ(kTouchActionMax, touch_action);
}

TEST(TouchActionRegionTest, GetWhiteListedTouchActionMapOverlapToNonZero) {
  TouchAction touch_action;
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionPanX, gfx::Rect(0, 0, 50, 50));
  touch_action_region.Union(kTouchActionPanRight, gfx::Rect(25, 25, 25, 25));
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10));
  EXPECT_EQ(kTouchActionPanX, touch_action);
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(30, 30));
  EXPECT_EQ(kTouchActionPanRight, touch_action);
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(60, 60));
  EXPECT_EQ(kTouchActionMax, touch_action);
}

TEST(TouchActionRegionTest, GetWhiteListedTouchActionEmptyMap) {
  TouchActionRegion touch_action_region;
  TouchAction touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10));
  EXPECT_EQ(kTouchActionMax, touch_action);
}

TEST(TouchActionRegionTest, GetWhiteListedTouchActionSingleMapEntry) {
  TouchActionRegion touch_action_region;
  TouchAction touch_action;
  touch_action_region.Union(kTouchActionPanUp, gfx::Rect(0, 0, 50, 50));
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10));
  EXPECT_EQ(kTouchActionPanUp, touch_action);
  touch_action =
      touch_action_region.GetWhiteListedTouchAction(gfx::Point(60, 60));
  EXPECT_EQ(kTouchActionMax, touch_action);
}

}  // namespace
}  // namespace cc
