// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/update/tray_update.h"

#include "ash/public/interfaces/update.mojom.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/test/ash_test.h"

namespace ash {

using TrayUpdateTest = AshTest;

// Tests that the update icon becomes visible when an update becomes
// available.
TEST_F(TrayUpdateTest, VisibilityAfterUpdate) {
  TrayUpdate* tray_update = GetPrimarySystemTray()->tray_update();

  // The system starts with no update pending, so the icon isn't visible.
  EXPECT_FALSE(tray_update->tray_view()->visible());

  // Simulate an update.
  Shell::Get()->system_tray_controller()->ShowUpdateIcon(
      mojom::UpdateSeverity::LOW, false);

  // Tray item is now visible.
  EXPECT_TRUE(tray_update->tray_view()->visible());
}

}  // namespace ash
