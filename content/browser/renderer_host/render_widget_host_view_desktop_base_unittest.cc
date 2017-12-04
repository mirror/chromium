// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_desktop_base_unittest.h"

#include "base/callback.h"
#include "content/browser/renderer_host/mock_render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_desktop_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

void RenderWidgetHostViewDesktopBase_TestShowHide(
    RenderWidgetHostViewDesktopBase* view) {
  RenderWidgetHostImpl* host = view->GetRenderWidgetHostImpl();

  // The host starts non-hidden.
  EXPECT_FALSE(host->is_hidden());

  // Hiding the view before it is first shown doesn't affect the host.
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());
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

void RenderWidgetHostViewDesktopBase_TestShowHideAndCapture(
    RenderWidgetHostViewDesktopBase* view,
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
  delegate->set_is_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when capture stops.
  delegate->set_is_captured(false);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // The host should be non-hidden when the view is shown.
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // Starting capture shouldn't affect the host.
  delegate->set_is_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should remain non-hidden when the view is hidden, because it is
  // being captured.
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());
}

void RenderWidgetHostViewDesktopBase_TestOcclusion(
    RenderWidgetHostViewDesktopBase* view,
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

void RenderWidgetHostViewDesktopBase_TestOcclusionAndCapture(
    RenderWidgetHostViewDesktopBase* view,
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
  delegate->set_is_captured(true);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // The host should be hidden when capture stops.
  delegate->set_is_captured(false);
  static_cast<RenderWidgetHostView*>(view)->CaptureStateChanged();
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  EXPECT_TRUE(host->is_hidden());

  // The host should be non-hidden when the view is unoccluded.
  set_is_occluded.Run(false);
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  EXPECT_FALSE(host->is_hidden());

  // Starting capture shouldn't affect the host.
  delegate->set_is_captured(true);
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
