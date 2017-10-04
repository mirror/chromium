// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_welcome_bubble.h"

#include "ash/system/palette/palette_tray.h"
#include "ash/system/palette/palette_welcome_bubble_test_api.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_test_helper.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/controls/button/image_button.h"

namespace ash {

class PaletteWelcomeBubbleTest : public AshTestBase {
 public:
  PaletteWelcomeBubbleTest() = default;
  ~PaletteWelcomeBubbleTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    welcome_bubble_ = std::make_unique<PaletteWelcomeBubble>(
        StatusAreaWidgetTestHelper::GetStatusAreaWidget()->palette_tray());
    test_api_ =
        std::make_unique<PaletteWelcomeBubbleTestAPI>(welcome_bubble_.get());
  }

 protected:
  std::unique_ptr<PaletteWelcomeBubble> welcome_bubble_;
  std::unique_ptr<PaletteWelcomeBubbleTestAPI> test_api_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaletteWelcomeBubbleTest);
};

TEST_F(PaletteWelcomeBubbleTest, Basic) {
  EXPECT_FALSE(welcome_bubble_->BubbleShown());

  welcome_bubble_->Show();
  EXPECT_TRUE(welcome_bubble_->BubbleShown());

  welcome_bubble_->Hide();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

TEST_F(PaletteWelcomeBubbleTest, CloseButton) {
  welcome_bubble_->Show();
  ASSERT_TRUE(welcome_bubble_->BubbleShown());
  ASSERT_TRUE(test_api_->GetBubbleCloseButton());

  GetEventGenerator().set_current_location(
      test_api_->GetBubbleCloseButton()->GetBoundsInScreen().CenterPoint());
  GetEventGenerator().ClickLeftButton();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

TEST_F(PaletteWelcomeBubbleTest, TapOutsideOfBubble) {
  welcome_bubble_->Show();
  ASSERT_TRUE(welcome_bubble_->BubbleShown());

  GetEventGenerator().set_current_location(
      test_api_->GetBubbleBounds().CenterPoint());
  GetEventGenerator().ClickLeftButton();
  EXPECT_TRUE(welcome_bubble_->BubbleShown());

  ASSERT_FALSE(test_api_->GetBubbleBounds().Contains(gfx::Point()));
  GetEventGenerator().set_current_location(gfx::Point());
  GetEventGenerator().ClickLeftButton();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

}  // namespace ash
