// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

#include "ash/display/window_tree_host_manager.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_state.h"
#include "base/command_line.h"

namespace ash {

class PersistentWindowControllerTest : public AshTestBase {
 protected:
  PersistentWindowControllerTest() = default;
  ~PersistentWindowControllerTest() override = default;

  // AshTestBase:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshEnablePersistentWindowBounds);
    AshTestBase::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PersistentWindowControllerTest);
};

TEST_F(PersistentWindowControllerTest, DisconnectDisplay) {
  UpdateDisplay("0+0-500x500,0+501-500x500");

  const int64_t primary_id = WindowTreeHostManager::GetPrimaryDisplayId();
  const int64_t secondary_id = display_manager()->GetSecondaryDisplay().id();

  display::ManagedDisplayInfo primary_info =
      display_manager()->GetDisplayInfo(primary_id);
  display::ManagedDisplayInfo secondary_info =
      display_manager()->GetDisplayInfo(secondary_id);
  aura::Window* w1 =
      CreateTestWindowInShellWithBounds(gfx::Rect(200, 0, 100, 200));
  aura::Window* w2 =
      CreateTestWindowInShellWithBounds(gfx::Rect(501, 0, 200, 100));
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(501, 0, 200, 100), w2->GetBoundsInScreen());

  // Disconnects secondary display.
  std::vector<display::ManagedDisplayInfo> display_info_list;
  display_info_list.push_back(primary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(1, 0, 200, 100), w2->GetBoundsInScreen());

  // Reconnects secondary display.
  display_info_list.push_back(secondary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(501, 0, 200, 100), w2->GetBoundsInScreen());

  // Disconnects primary display.
  display_info_list.clear();
  display_info_list.push_back(secondary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(1, 0, 200, 100), w2->GetBoundsInScreen());

  // Reconnects primary display.
  display_info_list.push_back(primary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(501, 0, 200, 100), w2->GetBoundsInScreen());

  // Disconnects secondary display.
  display_info_list.clear();
  display_info_list.push_back(primary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);

  // A third id which is different from primary and secondary.
  const int64_t third_id = secondary_id + 1;
  display::ManagedDisplayInfo third_info =
      display::CreateDisplayInfo(third_id, gfx::Rect(0, 501, 500, 500));
  // Connects another secondary display with |third_id|.
  display_info_list.push_back(third_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(1, 0, 200, 100), w2->GetBoundsInScreen());
  // Connects secondary display with |secondary_id|.
  display_info_list.clear();
  display_info_list.push_back(primary_info);
  display_info_list.push_back(secondary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(501, 0, 200, 100), w2->GetBoundsInScreen());

  // Disconnects secondary display.
  display_info_list.clear();
  display_info_list.push_back(primary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);

  // Sets |w2|'s bounds changed by user and then reconnects secondary display.
  wm::WindowState* w2_state = wm::GetWindowState(w2);
  w2_state->set_bounds_changed_by_user(true);
  display_info_list.push_back(secondary_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(1, 0, 200, 100), w2->GetBoundsInScreen());
}

TEST_F(PersistentWindowControllerTest, EnableAndDisableMirrorMode) {
  UpdateDisplay("0+0-500x500,0+501-500x500");

  aura::Window* w1 =
      CreateTestWindowInShellWithBounds(gfx::Rect(200, 0, 100, 200));
  aura::Window* w2 =
      CreateTestWindowInShellWithBounds(gfx::Rect(501, 0, 200, 100));
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(501, 0, 200, 100), w2->GetBoundsInScreen());

  // Enables mirror mode.
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_TRUE(display_manager()->IsInMirrorMode());
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(1, 0, 200, 100), w2->GetBoundsInScreen());
  // Disables mirror mode.
  display_manager()->SetMirrorMode(display::MirrorMode::kOff, base::nullopt);
  EXPECT_FALSE(display_manager()->IsInMirrorMode());
  EXPECT_EQ(gfx::Rect(200, 0, 100, 200), w1->GetBoundsInScreen());
  EXPECT_EQ(gfx::Rect(501, 0, 200, 100), w2->GetBoundsInScreen());
}

}  // namespace ash
