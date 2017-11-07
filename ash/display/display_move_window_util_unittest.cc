// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_move_window_util.h"

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_util.h"
#include "base/macros.h"
#include "ui/aura/test/test_windows.h"
#include "ui/display/display.h"
#include "ui/display/display_layout.h"
#include "ui/display/display_layout_builder.h"
#include "ui/display/manager/display_layout_store.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/display/test/display_manager_test_api.h"

namespace ash {

class DisplayMoveWindowUtilTest : public AshTestBase {
 public:
  DisplayMoveWindowUtilTest() = default;
  ~DisplayMoveWindowUtilTest() override = default;

  // AshTestBase:
  void SetUp() override {
    AshTestBase::SetUp();
    screen_ = display::Screen::GetScreen();
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
  display::Screen* screen_ = nullptr;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(DisplayMoveWindowUtilTest);
};

// Tests that window bounds are not changing after moving to another display. It
// is added as a child window to another root window with the same origin.
TEST_F(DisplayMoveWindowUtilTest, WindowBounds) {
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
  EXPECT_EQ(gfx::Rect(0, 0, 400, 300),
            display_manager()->GetDisplayAt(0).bounds());
  EXPECT_EQ(gfx::Rect(400, 0, 400, 300),
            display_manager()->GetDisplayAt(1).bounds());

  const gfx::Rect window_bounds(10, 20, 200, 100);
  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(window_bounds);
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kRight);
  EXPECT_EQ(gfx::Rect(410, 20, 200, 100), window->GetBoundsInScreen());
}

// A horizontal layout for three displays. They are perfectly horizontally
// aligned with no vertical offset.
TEST_F(DisplayMoveWindowUtilTest, ThreeDisplaysHorizontalLayout) {
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
  EXPECT_EQ(gfx::Rect(0, 0, 400, 300),
            display_manager()->GetDisplayAt(0).bounds());
  EXPECT_EQ(gfx::Rect(400, 0, 400, 300),
            display_manager()->GetDisplayAt(1).bounds());
  EXPECT_EQ(gfx::Rect(800, 0, 400, 300),
            display_manager()->GetDisplayAt(2).bounds());

  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(gfx::Rect(10, 20, 200, 100));
  ASSERT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kLeft);
  EXPECT_EQ(list[2], screen_->GetDisplayNearestWindow(window.get()).id());
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kLeft);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());

  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kRight);
  EXPECT_EQ(list[2], screen_->GetDisplayNearestWindow(window.get()).id());
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kRight);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  // No-op for above/below movement since no display is in the above/below of
  // primary display.
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kAbove);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kBelow);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());
}

// A vertical layout for three displays. They are laid out with horizontal
// offset.
TEST_F(DisplayMoveWindowUtilTest, ThreeDisplaysVerticalLayoutWithOffset) {
  int64_t primary_id = screen_->GetPrimaryDisplay().id();
  display::DisplayIdList list = display::test::CreateDisplayIdListN(
      3, primary_id, primary_id + 1, primary_id + 2);
  // Layout: [P]
  //          [2]
  //           [1]
  // Note that display [1] is completely in the right of display [p].
  display::DisplayLayoutBuilder builder(primary_id);
  builder.AddDisplayPlacement(list[1], list[2],
                              display::DisplayPlacement::BOTTOM, 200);
  builder.AddDisplayPlacement(list[2], primary_id,
                              display::DisplayPlacement::BOTTOM, 200);
  display_manager()->layout_store()->RegisterLayoutForDisplayIdList(
      list, builder.Build());
  UpdateDisplay("400x300,400x300,400x300");
  EXPECT_EQ(3U, display_manager()->GetNumDisplays());
  EXPECT_EQ(gfx::Rect(0, 0, 400, 300),
            display_manager()->GetDisplayAt(0).bounds());
  EXPECT_EQ(gfx::Rect(200, 300, 400, 300),
            display_manager()->GetDisplayAt(2).bounds());
  EXPECT_EQ(gfx::Rect(400, 600, 400, 300),
            display_manager()->GetDisplayAt(1).bounds());

  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(gfx::Rect(10, 20, 200, 100));
  ASSERT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kAbove);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kAbove);
  EXPECT_EQ(list[2], screen_->GetDisplayNearestWindow(window.get()).id());

  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kBelow);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kBelow);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  // Left/Right is able to be applied between display [p] and [1].
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kRight);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kLeft);
  EXPECT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());
}

// A more than three displays layout to verify the select closest strategy.
TEST_F(DisplayMoveWindowUtilTest, SelectClosestCandidate) {
  int64_t primary_id = screen_->GetPrimaryDisplay().id();
  // Layout:
  // +--------+
  // | 5      |----+
  // +---+----+ 3  |
  //     | 4  |--+----+----+
  //     +----+  | P  | 1  |
  //             +----+----+
  //                  | 2  |
  //                  +----+
  display::DisplayIdList list = display::test::CreateDisplayIdListN(
      6, primary_id, primary_id + 1, primary_id + 2, primary_id + 3,
      primary_id + 4, primary_id + 5);
  display::DisplayLayoutBuilder builder(primary_id);
  builder.AddDisplayPlacement(list[1], primary_id,
                              display::DisplayPlacement::RIGHT, 0);
  builder.AddDisplayPlacement(list[2], list[1],
                              display::DisplayPlacement::BOTTOM, 0);
  builder.AddDisplayPlacement(list[3], primary_id,
                              display::DisplayPlacement::TOP, -200);
  builder.AddDisplayPlacement(list[4], list[3], display::DisplayPlacement::LEFT,
                              200);
  builder.AddDisplayPlacement(list[5], list[3], display::DisplayPlacement::LEFT,
                              -100);
  display_manager()->layout_store()->RegisterLayoutForDisplayIdList(
      list, builder.Build());
  UpdateDisplay("400x300,400x300,400x300,400x300,400x300,800x300");
  EXPECT_EQ(6U, display_manager()->GetNumDisplays());
  EXPECT_EQ(gfx::Rect(0, 0, 400, 300),
            display_manager()->GetDisplayAt(0).bounds());
  EXPECT_EQ(gfx::Rect(400, 0, 400, 300),
            display_manager()->GetDisplayAt(1).bounds());
  EXPECT_EQ(gfx::Rect(400, 300, 400, 300),
            display_manager()->GetDisplayAt(2).bounds());
  EXPECT_EQ(gfx::Rect(-200, -300, 400, 300),
            display_manager()->GetDisplayAt(3).bounds());
  EXPECT_EQ(gfx::Rect(-600, -100, 400, 300),
            display_manager()->GetDisplayAt(4).bounds());
  EXPECT_EQ(gfx::Rect(-1000, -400, 800, 300),
            display_manager()->GetDisplayAt(5).bounds());

  std::unique_ptr<aura::Window> window =
      CreateTestWindowInPrimaryRootWindow(gfx::Rect(10, 20, 200, 100));
  ASSERT_EQ(list[0], screen_->GetDisplayNearestWindow(window.get()).id());

  // Moving window to left display, we have [4] and [5] candidate displays in
  // the left direction. They have the same left space but [4] is selected since
  // it is vertically closer to display [p].
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kLeft);
  EXPECT_EQ(list[4], screen_->GetDisplayNearestWindow(window.get()).id());

  // Moving window to left display, we don't have candidate displays in the left
  // direction. [1][2][3][p] are candidate displays in the cycled direction.
  // [1][2] have the same left space but [1] is selected since it is vertically
  // closer to display [4].
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kLeft);
  EXPECT_EQ(list[1], screen_->GetDisplayNearestWindow(window.get()).id());

  // Moving window to right display, we don't have candidate displays in the
  // right direction. [p][3][4][5] are candidate displays in the cycled
  // direction. [5] has the furthest right space so it is selected.
  HandleMoveWindowToDisplay(DisplayMoveWindowDirection::kRight);
  EXPECT_EQ(list[5], screen_->GetDisplayNearestWindow(window.get()).id());
}

}  // namespace ash
