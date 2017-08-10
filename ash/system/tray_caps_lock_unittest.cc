// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray_caps_lock.h"

#include "ash/session/test_session_controller_client.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"

namespace ash {
namespace {

using TrayCapsLockTest = AshTestBase;

// Tests that the icon becomes visible when the tray controller toggles it.
TEST_F(TrayCapsLockTest, Visibility) {
  TestSessionControllerClient* session = GetSessionControllerClient();
  // Ensure there is no PrefService for the active user. The pref used by
  // TrayCapTrayCapsLock is owned outside ash so the PrefService for tests
  // doesn't have it registered.
  // TODO(sammc): Register foreign-owned prefs in PrefServices created in
  // TestSessionControllerClient and remove this workaround.
  session->Reset();
  session->AddUserSession("user@example.com", user_manager::USER_TYPE_REGULAR,
                          true, false);

  SystemTray* tray = GetPrimarySystemTray();
  TrayCapsLock* caps_lock = SystemTrayTestApi(tray).tray_caps_lock();

  // By default the icon isn't visible.
  EXPECT_FALSE(caps_lock->tray_view()->visible());

  // Simulate turning on caps lock.
  caps_lock->OnCapsLockChanged(true);
  EXPECT_TRUE(caps_lock->tray_view()->visible());

  // Simulate turning off caps lock.
  caps_lock->OnCapsLockChanged(false);
  EXPECT_FALSE(caps_lock->tray_view()->visible());
}

}  // namespace
}  // namespace ash
