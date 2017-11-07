// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touch_action_filter.h"
#include "content/browser/renderer_host/event_with_latency_info.h"
#include "content/common/input/synthetic_web_input_event_builders.h"
#include "content/public/common/input_event_ack_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;

namespace content {
namespace {

const blink::WebGestureDevice kSourceDevice =
    blink::kWebGestureDeviceTouchscreen;

}  // namespace

static void PanTest(cc::TouchAction action,
                    float scroll_x,
                    float scroll_y,
                    float dx,
                    float dy,
                    float fling_x,
                    float fling_y,
                    float expected_dx,
                    float expected_dy,
                    float expected_fling_x,
                    float expected_fling_y) {
  TouchActionFilter filter;
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  {
    // Scrolls with no direction hint are permitted in the |action| direction.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(action);

    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(0, 0, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(dx, dy, 0,
                                                           kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_EQ(expected_dx, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(expected_dy, scroll_update.data.scroll_update.delta_y);

    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
  }

  {
    // Scrolls biased towards the touch-action axis are permitted.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(scroll_x, scroll_y,
                                                          kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(dx, dy, 0,
                                                           kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_EQ(expected_dx, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(expected_dy, scroll_update.data.scroll_update.delta_y);

    // Ensure that scrolls in the opposite direction are not filtered once
    // scrolling has started. (Once scrolling is started, the direction may
    // be reversed by the user even if scrolls that start in the reversed
    // direction are disallowed.
    WebGestureEvent scroll_update2 =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(-dx, -dy, 0,
                                                           kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update2));
    EXPECT_EQ(-expected_dx, scroll_update2.data.scroll_update.delta_x);
    EXPECT_EQ(-expected_dy, scroll_update2.data.scroll_update.delta_y);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        fling_x, fling_y, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&fling_start));
    EXPECT_EQ(expected_fling_x, fling_start.data.fling_start.velocity_x);
    EXPECT_EQ(expected_fling_y, fling_start.data.fling_start.velocity_y);
  }

  {
    // Scrolls biased towards the perpendicular of the touch-action axis are
    // suppressed entirely.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(scroll_y, scroll_x,
                                                          kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(dx, dy, 0,
                                                           kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_EQ(dx, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(dy, scroll_update.data.scroll_update.delta_y);

    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
  }
}

static void PanTestForUnidirectionalTouchAction(cc::TouchAction action,
                                                float scroll_x,
                                                float scroll_y) {
  TouchActionFilter filter;
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  {
    // Scrolls towards the touch-action direction are permitted.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(scroll_x, scroll_y,
                                                          kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(scroll_x, scroll_y,
                                                           0, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
  }

  {
    // Scrolls towards the exact opposite of the touch-action direction are
    // suppressed entirely.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-scroll_x, -scroll_y,
                                                          kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(-scroll_x, -scroll_y,
                                                           0, kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
  }

  {
    // Scrolls towards the diagonal opposite of the touch-action direction are
    // suppressed entirely.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(action);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(
            -scroll_x - scroll_y, -scroll_x - scroll_y, kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(
            -scroll_x - scroll_y, -scroll_x - scroll_y, 0, kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
  }
}

TEST(TouchActionFilterTest, SimpleFilter) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  const float kDeltaX = 5;
  const float kDeltaY = 10;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  // No events filtered by default.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));

  // cc::kTouchActionAuto doesn't cause any filtering.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

  // cc::kTouchActionNone filters out all scroll events, but no other events.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));

  // When a new touch sequence begins, the state is reset.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

  // Setting touch action doesn't impact any in-progress gestures.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

  // And the state is still cleared for the next gesture.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

  // Changing the touch action during a gesture has no effect.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
}

TEST(TouchActionFilterTest, Fling) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(5, 10, 0,
                                                         kSourceDevice);
  const float kFlingX = 7;
  const float kFlingY = -4;
  WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
      kFlingX, kFlingY, kSourceDevice);
  WebGestureEvent pad_fling = SyntheticWebGestureEventBuilder::BuildFling(
      kFlingX, kFlingY, blink::kWebGestureDeviceTouchpad);

  // cc::kTouchActionNone filters out fling events.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&fling_start));
  EXPECT_EQ(kFlingX, fling_start.data.fling_start.velocity_x);
  EXPECT_EQ(kFlingY, fling_start.data.fling_start.velocity_y);

  // touchpad flings aren't filtered.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pad_fling));
  EXPECT_TRUE(filter.FilterGestureEvent(&fling_start));
}

TEST(TouchActionFilterTest, PanLeft) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 7;
  const float kScrollY = 6;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanLeft, kScrollX, kScrollY, kDX, kDY, kFlingX,
          kFlingY, kDX, 0, kFlingX, 0);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanLeft, kScrollX, 0);
}

TEST(TouchActionFilterTest, PanRight) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = -7;
  const float kScrollY = 6;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanRight, kScrollX, kScrollY, kDX, kDY, kFlingX,
          kFlingY, kDX, 0, kFlingX, 0);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanRight, kScrollX, 0);
}

TEST(TouchActionFilterTest, PanX) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 7;
  const float kScrollY = 6;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanX, kScrollX, kScrollY, kDX, kDY, kFlingX, kFlingY,
          kDX, 0, kFlingX, 0);
}

TEST(TouchActionFilterTest, PanUp) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 6;
  const float kScrollY = 7;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanUp, kScrollX, kScrollY, kDX, kDY, kFlingX, kFlingY,
          0, kDY, 0, kFlingY);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanUp, 0, kScrollY);
}

TEST(TouchActionFilterTest, PanDown) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 6;
  const float kScrollY = -7;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanDown, kScrollX, kScrollY, kDX, kDY, kFlingX,
          kFlingY, 0, kDY, 0, kFlingY);
  PanTestForUnidirectionalTouchAction(cc::kTouchActionPanDown, 0, kScrollY);
}

TEST(TouchActionFilterTest, PanY) {
  const float kDX = 5;
  const float kDY = 10;
  const float kScrollX = 6;
  const float kScrollY = 7;
  const float kFlingX = 7;
  const float kFlingY = -4;

  PanTest(cc::kTouchActionPanY, kScrollX, kScrollY, kDX, kDY, kFlingX, kFlingY,
          0, kDY, 0, kFlingY);
}

TEST(TouchActionFilterTest, PanXY) {
  TouchActionFilter filter;
  const float kDX = 5;
  const float kDY = 10;
  const float kFlingX = 7;
  const float kFlingY = -4;

  {
    // Scrolls hinted in the X axis are permitted and unmodified.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-7, 6, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDX, kDY, 0,
                                                           kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_EQ(kDX, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(kDY, scroll_update.data.scroll_update.delta_y);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, kFlingY, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&fling_start));
    EXPECT_EQ(kFlingX, fling_start.data.fling_start.velocity_x);
    EXPECT_EQ(kFlingY, fling_start.data.fling_start.velocity_y);
  }

  {
    // Scrolls hinted in the Y axis are permitted and unmodified.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-6, 7, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDX, kDY, 0,
                                                           kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
    EXPECT_EQ(kDX, scroll_update.data.scroll_update.delta_x);
    EXPECT_EQ(kDY, scroll_update.data.scroll_update.delta_y);

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, kFlingY, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&fling_start));
    EXPECT_EQ(kFlingX, fling_start.data.fling_start.velocity_x);
    EXPECT_EQ(kFlingY, fling_start.data.fling_start.velocity_y);
  }

  {
    // A two-finger gesture is not allowed.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-6, 7, kSourceDevice,
                                                          2);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent scroll_update =
        SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDX, kDY, 0,
                                                           kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, kFlingY, kSourceDevice);
    EXPECT_TRUE(filter.FilterGestureEvent(&fling_start));
  }
}

TEST(TouchActionFilterTest, BitMath) {
  // Verify that the simple flag mixing properties we depend on are now
  // trivially true.
  EXPECT_EQ(cc::kTouchActionNone, cc::kTouchActionNone & cc::kTouchActionAuto);
  EXPECT_EQ(cc::kTouchActionNone, cc::kTouchActionPanY & cc::kTouchActionPanX);
  EXPECT_EQ(cc::kTouchActionPan, cc::kTouchActionAuto & cc::kTouchActionPan);
  EXPECT_EQ(cc::kTouchActionManipulation,
            cc::kTouchActionAuto & ~cc::kTouchActionDoubleTapZoom);
  EXPECT_EQ(cc::kTouchActionPanX,
            cc::kTouchActionPanLeft | cc::kTouchActionPanRight);
  EXPECT_EQ(cc::kTouchActionAuto,
            cc::kTouchActionManipulation | cc::kTouchActionDoubleTapZoom);
}

TEST(TouchActionFilterTest, MultiTouch) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  const float kDeltaX = 5;
  const float kDeltaY = 10;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  // For multiple points, the intersection is what matters.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));

  // Intersection of PAN_X and PAN_Y is NONE.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionPanX);
  filter.OnSetTouchAction(cc::kTouchActionPanY);
  filter.OnSetTouchAction(cc::kTouchActionPan);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
}

class TouchActionFilterPinchTest : public testing::Test {
 public:
  void RunTest(bool force_enable_zoom) {
    TouchActionFilter filter;
    filter.SetForceEnableZoom(force_enable_zoom);

    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice,
                                                          2);
    WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
        WebInputEvent::kGesturePinchBegin, kSourceDevice);
    WebGestureEvent pinch_update =
        SyntheticWebGestureEventBuilder::BuildPinchUpdate(1.2f, 5, 5, 0,
                                                          kSourceDevice);
    WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
        WebInputEvent::kGesturePinchEnd, kSourceDevice);
    WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
        WebInputEvent::kGestureScrollEnd, kSourceDevice);

    // Pinch is allowed with touch-action: auto.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

    // Pinch is not allowed with touch-action: none.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionNone);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));

    // Pinch is not allowed with touch-action: pan-x pan-y except for force
    // enable zoom.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPan);
    EXPECT_NE(filter.FilterGestureEvent(&scroll_begin), force_enable_zoom);
    EXPECT_NE(filter.FilterGestureEvent(&pinch_begin), force_enable_zoom);
    EXPECT_NE(filter.FilterGestureEvent(&pinch_update), force_enable_zoom);
    EXPECT_NE(filter.FilterGestureEvent(&pinch_end), force_enable_zoom);
    EXPECT_NE(filter.FilterGestureEvent(&scroll_end), force_enable_zoom);

    // Pinch is allowed with touch-action: manipulation.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionManipulation);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

    // Pinch state is automatically reset at the end of a scroll.
    filter.SetTouchActionToAuto();
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

    // Pinching is only computed at GestureScrollBegin time.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    filter.OnSetTouchAction(cc::kTouchActionNone);
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

    // Once a pinch has started, any change in state won't affect the pinch
    // gestures since it is computed in GestureScrollBegin.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionAuto);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    filter.OnSetTouchAction(cc::kTouchActionNone);
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

    // Scrolling is allowed when two fingers are down.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPinchZoom);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));

    // A pinch event sequence with only one pointer is equivalent to a scroll
    // gesture, so disallowed as a pinch gesture.
    scroll_begin.data.scroll_begin.pointer_count = 1;
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPinchZoom);
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_begin));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_update));
    EXPECT_TRUE(filter.FilterGestureEvent(&pinch_end));
    EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
  }
};

TEST_F(TouchActionFilterPinchTest, Pinch) {
  RunTest(false);
}

// Enables force enable zoom will override touch-action except for
// touch-action: none.
TEST_F(TouchActionFilterPinchTest, ForceEnableZoom) {
  RunTest(true);
}

TEST(TouchActionFilterTest, DoubleTapWithTouchActionAuto) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap_cancel = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapCancel, kSourceDevice);
  WebGestureEvent double_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureDoubleTap, kSourceDevice);

  // Double tap is allowed with touch action auto.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_down));
  EXPECT_FALSE(filter.FilterGestureEvent(&unconfirmed_tap));
  EXPECT_EQ(unconfirmed_tap.GetType(), WebInputEvent::kGestureTapUnconfirmed);
  // The tap cancel will come as part of the next touch sequence.
  filter.SetTouchActionToAuto();
  // Changing the touch action for the second tap doesn't effect the behaviour
  // of the event.
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_cancel));
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_down));
  EXPECT_FALSE(filter.FilterGestureEvent(&double_tap));
}

TEST(TouchActionFilterTest, DoubleTap) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap_cancel = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapCancel, kSourceDevice);
  WebGestureEvent double_tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureDoubleTap, kSourceDevice);

  // Double tap is disabled with any touch action other than auto.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionManipulation);
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_down));
  EXPECT_FALSE(filter.FilterGestureEvent(&unconfirmed_tap));
  EXPECT_EQ(WebInputEvent::kGestureTap, unconfirmed_tap.GetType());
  // Changing the touch action for the second tap doesn't effect the behaviour
  // of the event. The tap cancel will come as part of the next touch sequence.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionAuto);
  EXPECT_TRUE(filter.FilterGestureEvent(&tap_cancel));
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_down));
  EXPECT_FALSE(filter.FilterGestureEvent(&double_tap));
  EXPECT_EQ(WebInputEvent::kGestureTap, double_tap.GetType());
}

TEST(TouchActionFilterTest, SingleTapWithTouchActionAuto) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap1 = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  // Single tap is allowed with touch action auto.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_down));
  EXPECT_FALSE(filter.FilterGestureEvent(&unconfirmed_tap1));
  EXPECT_EQ(WebInputEvent::kGestureTapUnconfirmed, unconfirmed_tap1.GetType());
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));
}

TEST(TouchActionFilterTest, SingleTap) {
  TouchActionFilter filter;

  WebGestureEvent tap_down = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapDown, kSourceDevice);
  WebGestureEvent unconfirmed_tap1 = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTapUnconfirmed, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  // With touch action other than auto, tap unconfirmed is turned into tap.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_FALSE(filter.FilterGestureEvent(&tap_down));
  EXPECT_FALSE(filter.FilterGestureEvent(&unconfirmed_tap1));
  EXPECT_EQ(WebInputEvent::kGestureTap, unconfirmed_tap1.GetType());
  EXPECT_TRUE(filter.FilterGestureEvent(&tap));
}

TEST(TouchActionFilterTest, TouchActionResetsOnSetTouchActionToAuto) {
  TouchActionFilter filter;

  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);
  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));

  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));

  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
}

TEST(TouchActionFilterTest, TouchActionResetMidSequence) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  WebGestureEvent pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(1.2f, 5, 5, 0,
                                                        kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&pinch_update));

  // Even though the allowed action is auto after the reset, the remaining
  // scroll and pinch events should be suppressed.
  filter.SetTouchActionToAuto();
  EXPECT_TRUE(filter.FilterGestureEvent(&pinch_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));

  // A new scroll and pinch sequence should be allowed.
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));

  // Resetting from auto to auto mid-stream should have no effect.
  filter.SetTouchActionToAuto();
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
}

// crbug.com/771330, we test the case where we start with one finger scroll,
// then do a two finger pinch-zoom.
// Example: if the allowed touch action is pan-y and we start with one finger
// panning down, then later on (with the first finger still touching) we have
// the second finger to do pinch-zoom, our current behavior is to allow the
// pinch-zoom.
TEST(TouchActionFilterTest, SwitchFromOneFingerToTwo) {
  TouchActionFilter filter;

  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionPan);
  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(5, 3, 0,
                                                         kSourceDevice);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  WebGestureEvent pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(1.2f, 5, 5, 0,
                                                        kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  // Whether pinching is allowed or not is computed at the scroll_begin.
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
}

// crbug.com/771330: At this moment, if we start with two finger pinch-zoom, and
// then we lift one finger up and use the other finger to pan, it will be
// allowed.
TEST(TouchActionFilterTest, SwitchFromTwoFingersToOne) {
  TouchActionFilter filter;

  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionPinchZoom);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice, 2);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  WebGestureEvent pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(1.2f, 5, 5, 0,
                                                        kSourceDevice);
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(5, 0, 0,
                                                         kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
}

TEST(TouchActionFilterTest, ZeroVelocityFlingsConvertedToScrollEnd) {
  TouchActionFilter filter;
  const float kFlingX = 7;
  const float kFlingY = -4;

  {
    // Scrolls hinted mostly in the Y axis will suppress flings with a
    // component solely on the X axis, converting them to a GestureScrollEnd.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPanY);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-6, 7, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        kFlingX, 0, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&fling_start));
    EXPECT_EQ(WebInputEvent::kGestureScrollEnd, fling_start.GetType());
  }

  filter.SetTouchActionToAuto();

  {
    // Scrolls hinted mostly in the X axis will suppress flings with a
    // component solely on the Y axis, converting them to a GestureScrollEnd.
    filter.SetTouchActionToAuto();
    filter.OnSetTouchAction(cc::kTouchActionPanX);
    WebGestureEvent scroll_begin =
        SyntheticWebGestureEventBuilder::BuildScrollBegin(-7, 6, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));

    WebGestureEvent fling_start = SyntheticWebGestureEventBuilder::BuildFling(
        0, kFlingY, kSourceDevice);
    EXPECT_FALSE(filter.FilterGestureEvent(&fling_start));
    EXPECT_EQ(WebInputEvent::kGestureScrollEnd, fling_start.GetType());
  }
}

TEST(TouchActionFilterTest, TouchpadScroll) {
  TouchActionFilter filter;

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(
          2, 3, blink::kWebGestureDeviceTouchpad);

  // cc::kTouchActionNone filters out only touchscreen scroll events.
  filter.SetTouchActionToAuto();
  filter.OnSetTouchAction(cc::kTouchActionNone);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
}

TEST(TouchActionFilterTest, WhiteListedTouchActionMatchesHint) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionPanX);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(2, 3, kSourceDevice);
  const float kDeltaX = 5;
  const float kDeltaY = 10;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
  filter.ResetTouchAction();
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));
}

TEST(TouchActionFilterTest, WhiteListedTouchActionNotMatchHint) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionPanX);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(3, 2, kSourceDevice);
  const float kDeltaX = 10;
  const float kDeltaY = 5;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);

  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  // PanY is not allowed, the accumulated y scrolling resets at scroll update.
  EXPECT_EQ(0, scroll_update.data.scroll_update.delta_y);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
  filter.ResetTouchAction();
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));
}

TEST(TouchActionFilterTest, WhiteListedWithKnownRealTouchAction) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionPanX);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(3, 2, kSourceDevice);
  const float kDeltaX = 10;
  const float kDeltaY = 5;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  // PanY is not allowed, the accumulated y scrolling resets at scroll update.
  EXPECT_EQ(0, scroll_update.data.scroll_update.delta_y);

  const float kNewDeltaX = 15;
  const float kNewDeltaY = 7;
  filter.OnSetTouchAction(cc::kTouchActionPan);
  scroll_update = SyntheticWebGestureEventBuilder::BuildScrollUpdate(
      kNewDeltaX, kNewDeltaY, 0, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);
  WebGestureEvent tap = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureTap, kSourceDevice);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_EQ(kNewDeltaX, scroll_update.data.scroll_update.delta_x);
  // The kDeltaY = 5 is accumulated. At this moment we know the real touch
  // action, we apply the accumulated delta_y.
  EXPECT_EQ(kDeltaY + kNewDeltaY, scroll_update.data.scroll_update.delta_y);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
  filter.ResetTouchAction();
  EXPECT_FALSE(filter.FilterGestureEvent(&tap));
}

TEST(TouchActionFilterTest, WhiteListedTouchActionAuto) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionAuto);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(3, 2, kSourceDevice);
  const float kDeltaX = 10;
  const float kDeltaY = 5;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  // When touch action is auto, there is no need to accumulate any scroll delta.
  EXPECT_EQ(kDeltaX, scroll_update.data.scroll_update.delta_x);
  EXPECT_EQ(kDeltaY, scroll_update.data.scroll_update.delta_y);
}

TEST(TouchActionFilterTest, ScrollUpdateAfterPinchUpdate) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  // Setting whitelisted touch action auto is equivalent of setting real touch
  // action to auto.
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionAuto);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(3, 2, kSourceDevice, 2);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  const float pinch_scale = 1.2f;
  WebGestureEvent pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(pinch_scale, 5, 5, 0,
                                                        kSourceDevice);
  const float kDeltaX = 5;
  const float kDeltaY = 3;
  WebGestureEvent scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(kDeltaX, kDeltaY, 0,
                                                         kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
  EXPECT_EQ(scroll_update.data.scroll_update.delta_x, kDeltaX);
  EXPECT_EQ(scroll_update.data.scroll_update.delta_y, kDeltaY);
}

TEST(TouchActionFilterTest, AccumulatedPinchScaleAfterPinchUpdate) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionPinchZoom &
                                         cc::kTouchActionPanY);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(3, 2, kSourceDevice, 2);
  const float kFirstDeltaX = 2;
  const float kFirstDeltaY = 3;
  WebGestureEvent first_scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(
          kFirstDeltaX, kFirstDeltaY, 0, kSourceDevice);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  const float pinch_scale = 1.2f;
  WebGestureEvent pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(pinch_scale, 5, 5, 0,
                                                        kSourceDevice);
  const float kSecondDeltaX = 5;
  const float kSecondDeltaY = 10;
  WebGestureEvent second_scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(
          kSecondDeltaX, kSecondDeltaY, 0, kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&first_scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_EQ(pinch_update.data.pinch_update.scale, pinch_scale);
  EXPECT_TRUE(filter.FilterGestureEvent(&pinch_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_TRUE(filter.FilterGestureEvent(&second_scroll_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&scroll_end));
  // Even through the scroll update event is dropped, the accumulated pinch
  // scale should have been applied to the scroll delta;
  // This tests that the accumulated pinch scale won't be applied to the first
  // scroll update, only to the scroll updates occurs after that.
  EXPECT_EQ(second_scroll_update.data.scroll_update.delta_x,
            kSecondDeltaX * pinch_scale);
  EXPECT_EQ(second_scroll_update.data.scroll_update.delta_y,
            kSecondDeltaY * pinch_scale);
}

TEST(TouchActionFilterTest, AccumulatedPinchScaleWithKnownTouchAction) {
  TouchActionFilter filter;

  filter.ResetTouchAction();
  filter.OnReceiveWhiteListedTouchAction(cc::kTouchActionPinchZoom &
                                         cc::kTouchActionPanY);

  WebGestureEvent scroll_begin =
      SyntheticWebGestureEventBuilder::BuildScrollBegin(3, 2, kSourceDevice, 2);
  const float kFirstDeltaX = 2;
  const float kFirstDeltaY = 3;
  WebGestureEvent first_scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(
          kFirstDeltaX, kFirstDeltaY, 0, kSourceDevice);
  WebGestureEvent pinch_begin = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchBegin, kSourceDevice);
  const float first_pinch_scale = 1.2f;
  WebGestureEvent first_pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(first_pinch_scale, 5, 5,
                                                        0, kSourceDevice);
  WebGestureEvent ignored_pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(2.1f, 5, 5, 0,
                                                        kSourceDevice);
  ignored_pinch_update.data.pinch_update.zoom_disabled = true;
  const float kSecondDeltaX = 5;
  const float kSecondDeltaY = 10;
  WebGestureEvent second_scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(
          kSecondDeltaX, kSecondDeltaY, 0, kSourceDevice);
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_begin));
  EXPECT_TRUE(filter.FilterGestureEvent(&first_scroll_update));
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_begin));
  EXPECT_EQ(first_pinch_update.data.pinch_update.scale, first_pinch_scale);
  EXPECT_TRUE(filter.FilterGestureEvent(&first_pinch_update));
  // The |ignored_pinch_update| has |zoom_disable| = true, so the pinch scale
  // should never be accumulated.
  EXPECT_TRUE(filter.FilterGestureEvent(&ignored_pinch_update));
  EXPECT_TRUE(filter.FilterGestureEvent(&second_scroll_update));
  EXPECT_EQ(second_scroll_update.data.scroll_update.delta_x,
            kSecondDeltaX * first_pinch_scale);
  EXPECT_EQ(second_scroll_update.data.scroll_update.delta_y,
            kSecondDeltaY * first_pinch_scale);

  // Upon this point, both first_ and second_ scroll update is dropped, but both
  // scroll delta are accumulated. Also, the accumulated pinch scale (1.2f)
  // applies to the second scroll update only.
  // So we have accumulated pinch scale = 1.2, and accumulated scroll deltaX is
  // 2 + 1.2*5 = 8, and the accumulated scroll deltaY is 3 + 1.2*10 = 15.

  filter.OnSetTouchAction(cc::kTouchActionAuto);
  const float second_pinch_scale = 1.5f;
  WebGestureEvent second_pinch_update =
      SyntheticWebGestureEventBuilder::BuildPinchUpdate(second_pinch_scale, 5,
                                                        5, 0, kSourceDevice);
  const float kThirdDeltaX = 5;
  const float kThirdDeltaY = 10;
  WebGestureEvent third_scroll_update =
      SyntheticWebGestureEventBuilder::BuildScrollUpdate(
          kThirdDeltaX, kThirdDeltaY, 0, kSourceDevice);
  WebGestureEvent pinch_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGesturePinchEnd, kSourceDevice);
  WebGestureEvent scroll_end = SyntheticWebGestureEventBuilder::Build(
      WebInputEvent::kGestureScrollEnd, kSourceDevice);

  EXPECT_FALSE(filter.FilterGestureEvent(&second_pinch_update));
  EXPECT_EQ(second_pinch_update.data.pinch_update.scale,
            first_pinch_scale * second_pinch_scale);
  EXPECT_FALSE(filter.FilterGestureEvent(&third_scroll_update));
  // The scroll delta should have the accumulated scroll delta applied.
  EXPECT_EQ(third_scroll_update.data.scroll_update.delta_x,
            kThirdDeltaX + kFirstDeltaX + kSecondDeltaX * first_pinch_scale);
  EXPECT_EQ(third_scroll_update.data.scroll_update.delta_y,
            kThirdDeltaY + kFirstDeltaY + kSecondDeltaY * first_pinch_scale);
  EXPECT_FALSE(filter.FilterGestureEvent(&pinch_end));
  EXPECT_FALSE(filter.FilterGestureEvent(&scroll_end));
}

}  // namespace content
