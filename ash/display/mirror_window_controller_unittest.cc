// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/mirror_window_controller.h"

#include "ash/display/mirror_window_test_api.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/test/display_manager_test_api.h"

namespace ash {

namespace {
display::ManagedDisplayInfo CreateDisplayInfo(int64_t id,
                                              const gfx::Rect& bounds) {
  display::ManagedDisplayInfo info(
      id, base::StringPrintf("x-%d", static_cast<int>(id)), false);
  info.SetBounds(bounds);
  return info;
}

class MirrorOnBootTest : public AshTestBase {
 public:
  MirrorOnBootTest() {}
  ~MirrorOnBootTest() override {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        ::switches::kHostWindowBounds, "1+1-300x300,1+301-300x300");
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        ::switches::kEnableSoftwareMirroring);
    AshTestBase::SetUp();
  }
  void TearDown() override { AshTestBase::TearDown(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(MirrorOnBootTest);
};
}

using MirrorWindowControllerTest = AshTestBase;

// Make sure that the compositor based mirroring can switch from/to dock mode.
TEST_F(MirrorWindowControllerTest, DockMode) {
  const int64_t internal_id = 1;
  const int64_t external_id = 2;

  const display::ManagedDisplayInfo internal_display_info =
      CreateDisplayInfo(internal_id, gfx::Rect(0, 0, 500, 500));
  const display::ManagedDisplayInfo external_display_info =
      CreateDisplayInfo(external_id, gfx::Rect(1, 1, 100, 100));
  std::vector<display::ManagedDisplayInfo> display_info_list;

  display_manager()->SetMultiDisplayMode(display::DisplayManager::MIRRORING);

  // software mirroring.
  display_info_list.push_back(internal_display_info);
  display_info_list.push_back(external_display_info);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  const int64_t internal_display_id =
      display::test::DisplayManagerTestApi(display_manager())
          .SetFirstDisplayAsInternalDisplay();
  EXPECT_EQ(internal_id, internal_display_id);

  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_TRUE(display_manager()->IsInMirrorMode());
  EXPECT_EQ(external_id,
            display_manager()->GetMirroringDestinationDisplayIdList()[0]);

  // dock mode.
  display_info_list.clear();
  display_info_list.push_back(external_display_info);
  display_manager()->SetMultiDisplayMode(display::DisplayManager::MIRRORING);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_FALSE(display_manager()->IsInMirrorMode());

  // back to software mirroring.
  display_info_list.clear();
  display_info_list.push_back(internal_display_info);
  display_info_list.push_back(external_display_info);
  display_manager()->SetMultiDisplayMode(display::DisplayManager::MIRRORING);
  display_manager()->OnNativeDisplaysChanged(display_info_list);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_TRUE(display_manager()->IsInMirrorMode());
  EXPECT_EQ(external_id,
            display_manager()->GetMirroringDestinationDisplayIdList()[0]);
}

TEST_F(MirrorOnBootTest, MirrorOnBoot) {
  EXPECT_TRUE(display_manager()->IsInMirrorMode());

  // MirrorWindowController is not used in the MUS or MASH configs.
  if (Shell::GetAshConfig() != Config::CLASSIC)
    return;

  RunAllPendingInMessageLoop();
  MirrorWindowTestApi test_api;
  EXPECT_EQ(1U, test_api.GetHosts().size());
}

}  // namespace ash
