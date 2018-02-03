// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/flag_warning/flag_warning_tray.h"

#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/status_area_widget.h"
#include "ash/test/ash_test_base.h"

namespace ash {
namespace {

using FlagWarningTrayTest = AshTestBase;

TEST_F(FlagWarningTrayTest, Visibility) {
  // Button is always created.
  LogoutButtonTray* button = Shell::GetPrimaryRootWindowController()
                                 ->GetStatusAreaWidget()
                                 ->flag_warning_tray_for_testing();
  ASSERT_TRUE(button);

  // Warning should be visible in ash_unittests --mash, but not in regular
  // ash_unittests. The warning does not show for Config::MUS because mus will
  // roll out via experiment/Finch trial and showing the button would reveal
  // the experiment state to users.
  const bool is_mash = Shell::GetAshConfig() == Config::MASH;
  EXPECT_EQ(button->visible(), is_mash);
}

}  // namespace
}  // namespace ash
