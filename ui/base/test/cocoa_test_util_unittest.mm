// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/cocoa_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/test/ui_cocoa_test_helper.h"

using CocoaTestUtilTest = ui::CocoaTest;

namespace {

NSView* AutoreleasedViewWithFrame(int x, int y, int w, int h) {
  return [[[NSView alloc] initWithFrame:NSMakeRect(x, y, w, h)] autorelease];
}

void AdjustWidth(NSView* view, int delta) {
  NSRect r = view.frame;
  r.size.width += delta;
  [view setFrame:r];
}

}  // namespace

TEST_F(CocoaTestUtilTest, CorrectnessTest) {
  NSWindow* window = test_window();
  int width = NSWidth(window.contentView.frame);
  int height = NSHeight(window.contentView.frame);
  NSView* view_1 = AutoreleasedViewWithFrame(0, 0, width / 2, height);
  NSView* view_2 = AutoreleasedViewWithFrame(width / 2, 0, width / 2, height);
  NSView* view_3 = AutoreleasedViewWithFrame(0, 0, width / 2, height / 2);

  [view_1 addSubview:view_3];
  [window.contentView addSubview:view_1];
  [window.contentView addSubview:view_2];

  {
    // The original layout is well-formed.
    base::Optional<ui::ViewTreeProblemDetails> details =
        ui::ValidateViewTree(window.contentView);
    EXPECT_FALSE(details.has_value());
  }

  {
    // Make view_3 no longer contained within view_1.
    AdjustWidth(view_3, 1);
    base::Optional<ui::ViewTreeProblemDetails> details =
        ui::ValidateViewTree(window.contentView);
    ASSERT_TRUE(details.has_value());
    EXPECT_EQ(details->type, ui::ViewTreeProblemDetails::VIEW_OUTSIDE_PARENT);
    EXPECT_EQ(details->view_a, view_3);
    EXPECT_EQ(details->view_b, view_1);
    AdjustWidth(view_3, -1);
  }

  {
    // Make view_1 overlap view_2.
    AdjustWidth(view_1, 1);
    base::Optional<ui::ViewTreeProblemDetails> details =
        ui::ValidateViewTree(window.contentView);
    ASSERT_TRUE(details.has_value());
    EXPECT_EQ(details->type, ui::ViewTreeProblemDetails::VIEWS_OVERLAP);

    // Since there's no specified order for |view_a| and |view_b| for
    // VIEWS_OVERLAP, check that |view_1| and |view_2| both appear exactly once
    // in |view_a| and |view_b|.
    EXPECT_TRUE(details->view_a == view_1 || details->view_a == view_2);
    EXPECT_TRUE(details->view_b == view_1 || details->view_b == view_2);
    EXPECT_NE(details->view_a, details->view_b);
  }
}
