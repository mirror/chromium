// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_move_window_controller.h"

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_util.h"
#include "ui/aura/test/test_windows.h"
#include "ui/display/display.h"
#include "ui/display/display_layout.h"
#include "ui/display/display_layout_builder.h"
#include "ui/display/manager/display_layout_store.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/display/test/display_manager_test_api.h"

namespace ash {

using Direction = DisplayMoveWindowController::Direction;

class DisplayMoveWindowControllerTest : public AshTestBase {
 public:
  DisplayMoveWindowControllerTest() = default;
  ~DisplayMoveWindowControllerTest() override = default;

  // AshTestBase:
  void SetUp() override {
    AshTestBase::SetUp();
    screen_ = display::Screen::GetScreen();
    controller_ = Shell::Get()->display_move_window_controller();
  }

  std::unique_ptr<aura::Window> CreateTestWindowInPrimaryRootWindow(
      const gfx::Rect& bounds) {
    std::unique_ptr<aura::Window> window(aura::test::CreateTestWindowWithBounds(
        bounds, Shell::GetPrimaryRootWindow()));
    ParentWindowInPrimaryRootWindow(window.get());
    wm::ActivateWindow(window.get());
    return window;
  }

 protected:
  display::Screen* screen_ = nullptr;                  // Not owned.
  DisplayMoveWindowController* controller_ = nullptr;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(DisplayMoveWindowControllerTest);
};

TEST_F(DisplayMoveWindowControllerTest, WindowBounds) {
  int64_t primary_id = screen_->GetPrimaryDisplay().id();
  display::DisplayIdList list =
      display::test::CreateDisplayIdList2(primary_id, primary_id + 1);
  // Layout: [p][1]
  display::DisplayLayoutBuilder builder(primary_id);
  builder.AddDisplayPlacement(list[1], primary_id,
                              display::DisplayPlacement::RIGHT, 0);
  display_manager()->layout_store()->RegisterLayoutForDisplayIdList(
      list, builder.Build());
  UpdateDisplay("400x300,400x300");
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 400x300",
            display_manager()->GetDisplayAt(0).bounds().ToString());
  EXPECT_EQ("400,0 400x300",
            display_manager()->GetDisplayAt(1).bounds().ToString());

  const gfx::Rect window_bounds(10, 20, 200, 100);
  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(window_bounds);
  controller_->HandleMoveWindowToDisplay(Direction::RIGHT);
  EXPECT_EQ("410,20 200x100", window->GetBoundsInScreen().ToString());
}

TEST_F(DisplayMoveWindowControllerTest, ThreeDisplaysHorizontalLayout) {
  int64_t primary_id = screen_->GetPrimaryDisplay().id();
  display::DisplayIdList list = display::test::CreateDisplayIdListN(
      3, primary_id, primary_id + 1, primary_id + 2);
  // Layout: [p][1][2]
  display::DisplayLayoutBuilder builder(primary_id);
  builder.AddDisplayPlacement(list[1], primary_id,
                              display::DisplayPlacement::RIGHT, 0);
  builder.AddDisplayPlacement(list[2], list[1],
                              display::DisplayPlacement::RIGHT, 0);
  display_manager()->layout_store()->RegisterLayoutForDisplayIdList(
      list, builder.Build());
  UpdateDisplay("400x300,400x300,400x300");
  EXPECT_EQ(3U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 400x300",
            display_manager()->GetDisplayAt(0).bounds().ToString());
  EXPECT_EQ("400,0 400x300",
            display_manager()->GetDisplayAt(1).bounds().ToString());
  EXPECT_EQ("800,0 400x300",
            display_manager()->GetDisplayAt(2).bounds().ToString());

  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(gfx::Rect(10, 20, 200, 100));
  ASSERT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  controller_->HandleMoveWindowToDisplay(Direction::LEFT);
  EXPECT_EQ(list[2], screen_->GetDisplayNearestWindow(window.get()).id());
  controller_->HandleMoveWindowToDisplay(Direction::LEFT);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());

  controller_->HandleMoveWindowToDisplay(Direction::RIGHT);
  EXPECT_EQ(list[2], screen_->GetDisplayNearestWindow(window.get()).id());
  controller_->HandleMoveWindowToDisplay(Direction::RIGHT);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  controller_->HandleMoveWindowToDisplay(Direction::UP);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());
  controller_->HandleMoveWindowToDisplay(Direction::DOWN);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());
}

TEST_F(DisplayMoveWindowControllerTest, ThreeDisplaysVerticalLayoutWithOffset) {
  int64_t primary_id = screen_->GetPrimaryDisplay().id();
  display::DisplayIdList list = display::test::CreateDisplayIdListN(
      3, primary_id, primary_id + 1, primary_id + 2);
  // Layout: [P]
  //          [2]
  //           [1]
  display::DisplayLayoutBuilder builder(primary_id);
  builder.AddDisplayPlacement(list[1], list[2],
                              display::DisplayPlacement::BOTTOM, 200);
  builder.AddDisplayPlacement(list[2], primary_id,
                              display::DisplayPlacement::BOTTOM, 200);
  display_manager()->layout_store()->RegisterLayoutForDisplayIdList(
      list, builder.Build());
  UpdateDisplay("400x300,400x300,400x300");
  EXPECT_EQ(3U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 400x300",
            display_manager()->GetDisplayAt(0).bounds().ToString());
  EXPECT_EQ("200,300 400x300",
            display_manager()->GetDisplayAt(2).bounds().ToString());
  EXPECT_EQ("400,600 400x300",
            display_manager()->GetDisplayAt(1).bounds().ToString());

  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(gfx::Rect(10, 20, 200, 100));
  ASSERT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  controller_->HandleMoveWindowToDisplay(Direction::UP);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());
  controller_->HandleMoveWindowToDisplay(Direction::UP);
  EXPECT_EQ(list[2], screen_->GetDisplayNearestWindow(window.get()).id());

  controller_->HandleMoveWindowToDisplay(Direction::DOWN);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());
  controller_->HandleMoveWindowToDisplay(Direction::DOWN);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  controller_->HandleMoveWindowToDisplay(Direction::RIGHT);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());
  controller_->HandleMoveWindowToDisplay(Direction::LEFT);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());
}

}  // namespace ash
