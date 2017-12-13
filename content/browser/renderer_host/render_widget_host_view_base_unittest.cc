// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_base_unittest.h"

#include "base/callback.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/test/mock_render_widget_host_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/screen_orientation/WebScreenOrientationType.h"
#include "ui/display/display.h"

namespace content {

namespace {

display::Display CreateDisplay(int width, int height, int angle) {
  display::Display display;
  display.SetRotationAsDegree(angle);
  display.set_bounds(gfx::Rect(width, height));

  return display;
}

}  // namespace

TEST(RenderWidgetHostViewBaseTest, OrientationTypeForMobile) {
  // Square display (width == height).
  {
    display::Display display = CreateDisplay(100, 100, 0);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_PORTRAIT_PRIMARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(200, 200, 90);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(0, 0, 180);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_PORTRAIT_SECONDARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(10000, 10000, 270);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_LANDSCAPE_SECONDARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));
  }

  // natural width > natural height.
  {
    display::Display display = CreateDisplay(1, 0, 0);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(19999, 20000, 90);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_PORTRAIT_SECONDARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(200, 100, 180);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_LANDSCAPE_SECONDARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(1, 10000, 270);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_PORTRAIT_PRIMARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));
  }

  // natural width < natural height.
  {
    display::Display display = CreateDisplay(0, 1, 0);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_PORTRAIT_PRIMARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(20000, 19999, 90);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(100, 200, 180);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_PORTRAIT_SECONDARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));

    display = CreateDisplay(10000, 1, 270);
    EXPECT_EQ(SCREEN_ORIENTATION_VALUES_LANDSCAPE_SECONDARY,
              RenderWidgetHostViewBase::GetOrientationTypeForMobile(display));
  }
}

TEST(RenderWidgetHostViewBaseTest, OrientationTypeForDesktop) {
  // On Desktop, the primary orientation is the first computed one so a test
  // similar to OrientationTypeForMobile is not possible.
  // Instead this test will only check one configuration and verify that the
  // method reports two landscape and two portrait orientations with one primary
  // and one secondary for each.

  // natural width > natural height.
  {
    display::Display display = CreateDisplay(1, 0, 0);
    ScreenOrientationValues landscape_1 =
        RenderWidgetHostViewBase::GetOrientationTypeForDesktop(display);
    EXPECT_TRUE(landscape_1 == SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY ||
                landscape_1 == SCREEN_ORIENTATION_VALUES_LANDSCAPE_SECONDARY);

    display = CreateDisplay(200, 100, 180);
    ScreenOrientationValues landscape_2 =
        RenderWidgetHostViewBase::GetOrientationTypeForDesktop(display);
    EXPECT_TRUE(landscape_2 == SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY ||
                landscape_2 == SCREEN_ORIENTATION_VALUES_LANDSCAPE_SECONDARY);

    EXPECT_NE(landscape_1, landscape_2);

    display = CreateDisplay(19999, 20000, 90);
    ScreenOrientationValues portrait_1 =
        RenderWidgetHostViewBase::GetOrientationTypeForDesktop(display);
    EXPECT_TRUE(portrait_1 == SCREEN_ORIENTATION_VALUES_PORTRAIT_PRIMARY ||
                portrait_1 == SCREEN_ORIENTATION_VALUES_PORTRAIT_SECONDARY);

    display = CreateDisplay(1, 10000, 270);
    ScreenOrientationValues portrait_2 =
        RenderWidgetHostViewBase::GetOrientationTypeForDesktop(display);
    EXPECT_TRUE(portrait_2 == SCREEN_ORIENTATION_VALUES_PORTRAIT_PRIMARY ||
                portrait_2 == SCREEN_ORIENTATION_VALUES_PORTRAIT_SECONDARY);

    EXPECT_NE(portrait_1, portrait_2);

  }
}

void RenderWidgetHostViewBase_TestShowHide(RenderWidgetHostViewBase* view) {
  RenderWidgetHostImpl* host = view->GetRenderWidgetHostImpl();

  // The host starts non-hidden.
  EXPECT_FALSE(host->is_hidden());

  // Hiding the view before it is first shown hides the host.
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // Showing the view shows the host.
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // Hiding the view hides the host.
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // Showing the view shows the host.
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());
}

void RenderWidgetHostViewBase_TestShowHideAndCapture(
    RenderWidgetHostViewBase* view,
    MockRenderWidgetHostDelegate* delegate) {
  RenderWidgetHostImpl* host = view->GetRenderWidgetHostImpl();

  // The host starts non-hidden.
  EXPECT_FALSE(host->is_hidden());
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when the view is hidden.
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // The host should be non-hidden when capture starts.
  delegate->set_is_being_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when capture stops.
  delegate->set_is_being_captured(false);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // The host should be non-hidden when the view is shown.
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // Starting capture shouldn't affect the host.
  delegate->set_is_being_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should remain non-hidden when the view is hidden, because it is
  // being captured.
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());
}

void RenderWidgetHostViewBase_TestOcclusion(
    RenderWidgetHostViewBase* view,
    const SetIsOccludedCallback& set_is_occluded) {
  RenderWidgetHostImpl* host = view->GetRenderWidgetHostImpl();

  // The host starts non-hidden.
  EXPECT_FALSE(host->is_hidden());
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when the view is occluded.
  set_is_occluded.Run(true);
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());
  set_is_occluded.Run(false);
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // Hiding and showing the view when it's occluded shouldn't affect the host.
  set_is_occluded.Run(true);
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());
  view->Show();
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());
}

void RenderWidgetHostViewBase_TestOcclusionAndCapture(
    RenderWidgetHostViewBase* view,
    MockRenderWidgetHostDelegate* delegate,
    const SetIsOccludedCallback& set_is_occluded) {
  RenderWidgetHostImpl* host = view->GetRenderWidgetHostImpl();

  // The host starts non-hidden.
  EXPECT_FALSE(host->is_hidden());
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when the view is occluded.
  set_is_occluded.Run(true);
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // The host should be non-hidden when capture starts.
  delegate->set_is_being_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when capture stops.
  delegate->set_is_being_captured(false);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // The host should be non-hidden when the view is unoccluded.
  set_is_occluded.Run(false);
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // Starting capture shouldn't affect the host.
  delegate->set_is_being_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should remain non-hidden when the view is occcluded, because it is
  // being captured.
  set_is_occluded.Run(true);
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());
}

}  // namespace content
