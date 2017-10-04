// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_welcome_bubble.h"

#include "ash/system/palette/palette_tray.h"
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
  }

  // Returns the bounds of the bubble view if it exists.
  gfx::Rect GetBubbleBounds() {
    if (welcome_bubble_->bubble_view_)
      return welcome_bubble_->bubble_view_->GetBoundsInScreen();

    return gfx::Rect();
  }

 protected:
  std::unique_ptr<PaletteWelcomeBubble> welcome_bubble_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaletteWelcomeBubbleTest);
};

// Test the basic Show/Hide functions work.
TEST_F(PaletteWelcomeBubbleTest, Basic) {
  EXPECT_FALSE(welcome_bubble_->BubbleShown());

  welcome_bubble_->Show();
  EXPECT_TRUE(welcome_bubble_->BubbleShown());

  welcome_bubble_->Hide();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

// Verify that tapping the close button on the welcome bubble closes the bubble.
TEST_F(PaletteWelcomeBubbleTest, CloseButton) {
  welcome_bubble_->Show();
  ASSERT_TRUE(welcome_bubble_->BubbleShown());
  ASSERT_TRUE(welcome_bubble_->GetCloseButtonForTest());

  GetEventGenerator().set_current_location(
      welcome_bubble_->GetCloseButtonForTest()
          ->GetBoundsInScreen()
          .CenterPoint());
  GetEventGenerator().ClickLeftButton();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

// Verify that tapping on the screen outside of the welcome bubble closes the
// bubble.
TEST_F(PaletteWelcomeBubbleTest, TapOutsideOfBubble) {
  welcome_bubble_->Show();
  ASSERT_TRUE(welcome_bubble_->BubbleShown());

  // The bubble remains open if a tap occurs on the bubble.
  GetEventGenerator().set_current_location(GetBubbleBounds().CenterPoint());
  GetEventGenerator().ClickLeftButton();
  EXPECT_TRUE(welcome_bubble_->BubbleShown());

  ASSERT_FALSE(GetBubbleBounds().Contains(gfx::Point()));
  GetEventGenerator().set_current_location(gfx::Point());
  GetEventGenerator().ClickLeftButton();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

}  // namespace ash
