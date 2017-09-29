// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/progress_bar.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace views {

TEST(ProgressBarTest, Accessibility) {
  ProgressBar bar;
  bar.SetValue(0.62);

  ui::AXNodeData node_data;
  bar.GetAccessibleNodeData(&node_data);
  EXPECT_EQ(ui::AX_ROLE_PROGRESS_INDICATOR, node_data.role);
  EXPECT_EQ(base::string16(), node_data.GetString16Attribute(ui::AX_ATTR_NAME));
  EXPECT_FALSE(node_data.HasIntAttribute(ui::AX_ATTR_RESTRICTION));
}

TEST(ProgressBarTest, GetForegroundColorReturnsNativeThemeColorByDefault) {
  ProgressBar bar;
  EXPECT_EQ(bar.GetForegroundColor(),
            bar.GetNativeTheme()->GetSystemColor(
                ui::NativeTheme::kColorId_ProminentButtonColor));
}

TEST(ProgressBarTest,
     GetBackgroundColorReturnsBlendedForegroundColorByDefault) {
  ProgressBar bar;
  EXPECT_EQ(bar.GetBackgroundColor(), color_utils::BlendTowardOppositeLuma(
                                          bar.GetForegroundColor(), 0xCC));
}

TEST(ProgressBarTest, SetForegroundColorOverridesDefaultColor) {
  ProgressBar bar;
  SkColor expected = SkColorSetARGB(1, 2, 3, 4);
  bar.SetForegroundColor(expected);
  EXPECT_EQ(bar.GetForegroundColor(), expected);
}

TEST(ProgressBarTest, SetBackgroundColorOverridesDefaultColor) {
  ProgressBar bar;
  SkColor expected = SkColorSetARGB(1, 2, 3, 4);
  bar.SetBackgroundColor(expected);
  EXPECT_EQ(bar.GetBackgroundColor(), expected);
}

}  // namespace views
