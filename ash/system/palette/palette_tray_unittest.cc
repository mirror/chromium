// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_tray.h"

#include "ash/ash_switches.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/tray_bubble_wrapper.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/shell_test_api.h"
#include "ash/test/status_area_widget_test_helper.h"
#include "ash/test/test_palette_delegate.h"
#include "base/command_line.h"
#include "ui/events/event.h"

namespace ash {

class PaletteTrayTest : public test::AshTestBase {
 public:
  PaletteTrayTest() {}
  ~PaletteTrayTest() override {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshForceEnableStylusTools);
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshEnablePaletteOnAllDisplays);

    AshTestBase::SetUp();

    palette_tray_ =
        StatusAreaWidgetTestHelper::GetStatusAreaWidget()->palette_tray();
    palette_tool_manager_ = palette_tray_->palette_tool_manager_.get();

    test::ShellTestApi().SetPaletteDelegate(
        base::MakeUnique<TestPaletteDelegate>());
  }

  TrayBubbleWrapper* GetTrayBubbleWrapper() {
    return palette_tray_->bubble_.get();
  }

  // Performs a tap on the palette tray button.
  void PerformTap() {
    ui::GestureEvent tap(0, 0, 0, base::TimeTicks(),
                         ui::GestureEventDetails(ui::ET_GESTURE_TAP));
    palette_tray_->PerformAction(tap);
  }

 protected:
  // These are not owned.
  PaletteTray* palette_tray_ = nullptr;
  PaletteToolManager* palette_tool_manager_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaletteTrayTest);
};

// Verify the palette tray button exists and is visible with the flags we added
// during setup.
TEST_F(PaletteTrayTest, PaletteTrayIsVisible) {
  ASSERT_TRUE(palette_tray_);
  EXPECT_TRUE(palette_tray_->visible());
}

TEST_F(PaletteTrayTest, PaletteTrayWorkflow) {
  // Verify the palette tray button is not active, and the palette tray bubble
  // is not shown initially.
  EXPECT_FALSE(palette_tray_->is_active());
  EXPECT_FALSE(GetTrayBubbleWrapper());

  // Verify that by tapping the palette tray button, the button will become
  // active and the palette tray bubble will be open.
  PerformTap();
  EXPECT_TRUE(palette_tray_->is_active());
  EXPECT_TRUE(GetTrayBubbleWrapper());

  // Verify that activating a mode tool will close the palette tray bubble, but
  // leave the palette tray button active.
  palette_tool_manager_->ActivateTool(PaletteToolId::LASER_POINTER);
  EXPECT_TRUE(
      palette_tool_manager_->IsToolActive(PaletteToolId::LASER_POINTER));
  EXPECT_TRUE(palette_tray_->is_active());
  EXPECT_FALSE(GetTrayBubbleWrapper());

  // Verify that tapping the palette tray while a tool is active will deactivate
  // the tool, and the palette tray button will not be active.
  PerformTap();
  EXPECT_FALSE(palette_tray_->is_active());
  EXPECT_FALSE(
      palette_tool_manager_->IsToolActive(PaletteToolId::LASER_POINTER));

  // Verify that activating a action tool will close the palette tray bubble and
  // the palette tray button is will not be active.
  PerformTap();
  ASSERT_TRUE(GetTrayBubbleWrapper());
  palette_tool_manager_->ActivateTool(PaletteToolId::CAPTURE_SCREEN);
  EXPECT_FALSE(
      palette_tool_manager_->IsToolActive(PaletteToolId::CAPTURE_SCREEN));
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(GetTrayBubbleWrapper());
  EXPECT_FALSE(palette_tray_->is_active());
}

}  // namespace ash
