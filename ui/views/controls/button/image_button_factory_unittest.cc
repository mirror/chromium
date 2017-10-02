// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/image_button_factory.h"

#include "build/build_config.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/animation/test/ink_drop_host_view_test_api.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/test/views_test_base.h"

namespace views {

typedef ViewsTestBase ImageButtonFactoryTest;

TEST_F(ImageButtonFactoryTest, CreateVectorImageButton) {
  std::unique_ptr<ImageButton> button(CreateVectorImageButton(nullptr));
  EXPECT_EQ(ImageButton::ALIGN_CENTER, button->h_alignment_);
  EXPECT_EQ(ImageButton::ALIGN_MIDDLE, button->v_alignment_);
  EXPECT_TRUE(test::InkDropHostViewTestApi(button.get()).HasGestureHandler());

// Default to platform focus behavior.
#if defined(OS_MACOSX)
  EXPECT_EQ(View::FocusBehavior::ACCESSIBLE_ONLY, button->focus_behavior());
#else
  EXPECT_EQ(View::FocusBehavior::ALWAYS, button->focus_behavior());
#endif  // defined(OS_MACOSX)
}

TEST_F(ImageButtonFactoryTest, SetImageFromVectorIcon) {
  std::unique_ptr<ImageButton> button(CreateVectorImageButton(nullptr));
  SetImageFromVectorIcon(button.get(), vector_icons::kClose16Icon, SK_ColorRED);
  EXPECT_FALSE(button->GetImage(Button::STATE_NORMAL).isNull());
  EXPECT_FALSE(button->GetImage(Button::STATE_DISABLED).isNull());
  EXPECT_EQ(color_utils::DeriveDefaultIconColor(SK_ColorRED),
            button->GetInkDropBaseColor());

  // Default to black.
  SetImageFromVectorIcon(button.get(), vector_icons::kClose16Icon);
  EXPECT_EQ(color_utils::DeriveDefaultIconColor(SK_ColorBLACK),
            button->GetInkDropBaseColor());
}

}  // views
