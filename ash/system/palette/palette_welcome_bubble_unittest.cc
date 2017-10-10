// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_welcome_bubble.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/shell.h"
#include "ash/system/palette/palette_tray.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_test_helper.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
#include "components/prefs/pref_service.h"
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
    // Manually initialize the prefs here since |welcome_bubble_| is created
    // after Shell notifies its observers of local state pref initialization.
    welcome_bubble_->OnLocalStatePrefServiceInitialized(
        Shell::Get()->GetLocalStatePrefService());
  }

  void TearDown() override {
    welcome_bubble_.reset();
    AshTestBase::TearDown();
  }

  void ShowBubble() { welcome_bubble_->Show(); }
  void HideBubble() { welcome_bubble_->Hide(); }

 protected:
  std::unique_ptr<PaletteWelcomeBubble> welcome_bubble_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaletteWelcomeBubbleTest);
};

// Test the basic Show/Hide functions work.
TEST_F(PaletteWelcomeBubbleTest, Basic) {
  EXPECT_FALSE(welcome_bubble_->BubbleShown());

  ShowBubble();
  EXPECT_TRUE(welcome_bubble_->BubbleShown());

  HideBubble();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());

  // Verify that the pref changes after the bubble is hidden.
  EXPECT_TRUE(Shell::Get()->GetLocalStatePrefService()->GetBoolean(
      prefs::kShownPaletteWelcomeBubble));
}

// Verify if the bubble has been show before, it will not be shown again.
TEST_F(PaletteWelcomeBubbleTest, ShowIfNeeded) {
  Shell::Get()->GetLocalStatePrefService()->SetBoolean(
      prefs::kShownPaletteWelcomeBubble, true);

  welcome_bubble_->ShowIfNeeded();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

// Verify that tapping the close button on the welcome bubble closes the bubble.
TEST_F(PaletteWelcomeBubbleTest, CloseButton) {
  ShowBubble();
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
  ShowBubble();
  ASSERT_TRUE(welcome_bubble_->BubbleShown());

  // The bubble remains open if a tap occurs on the bubble.
  GetEventGenerator().set_current_location(
      welcome_bubble_->GetBubbleBoundsForTest().CenterPoint());
  GetEventGenerator().ClickLeftButton();
  EXPECT_TRUE(welcome_bubble_->BubbleShown());

  ASSERT_FALSE(
      welcome_bubble_->GetBubbleBoundsForTest().Contains(gfx::Point()));
  GetEventGenerator().set_current_location(gfx::Point());
  GetEventGenerator().ClickLeftButton();
  EXPECT_FALSE(welcome_bubble_->BubbleShown());
}

}  // namespace ash
