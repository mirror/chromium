// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_desktop_base_unittest.h"

#include "base/callback.h"
#include "content/browser/renderer_host/render_widget_host_view_desktop_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

void RenderWidgetHostViewDesktopBase_TestShowHide(
    RenderWidgetHostViewDesktopBase* view) {
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
}

void RenderWidgetHostViewDesktopBase_TestOcclusion(
    RenderWidgetHostViewDesktopBase* view,
    const SetIsOccludedCallback& set_is_occluded) {
  view->Show();
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());

  set_is_occluded.Run(true);
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
  set_is_occluded.Run(false);
  EXPECT_EQ(Visibility::VISIBLE, view->GetVisibility());
  set_is_occluded.Run(true);
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());

  view->Hide();
  EXPECT_EQ(Visibility::HIDDEN, view->GetVisibility());
  view->Show();
  EXPECT_EQ(Visibility::OCCLUDED, view->GetVisibility());
}

}  // namespace content
