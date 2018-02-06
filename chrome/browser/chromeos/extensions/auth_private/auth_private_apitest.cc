// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/extensions/auth_private/auth_private_api.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/login/lock/screen_locker_tester.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/chromeos_switches.h"
#include "components/session_manager/core/session_manager.h"
#include "components/session_manager/session_manager_types.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"

namespace chromeos {

class AuthPrivateApiTest : public ExtensionApiTest {};

class AuthPrivateApiSigninTest : public ExtensionApiTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kLoginManager);
    command_line->AppendSwitch(switches::kForceLoginManagerInTests);
    ExtensionApiTest::SetUpCommandLine(command_line);
  }
};

// User profile - logged in, screen not locked.
IN_PROC_BROWSER_TEST_F(AuthPrivateApiTest, User) {
  ASSERT_TRUE(RunComponentExtensionTest("auth_private/user")) << message_;
}

// Signin profile - not logged in, screen not locked.
IN_PROC_BROWSER_TEST_F(AuthPrivateApiSigninTest, Signin) {
  ASSERT_TRUE(RunComponentExtensionTest("auth_private/signin")) << message_;
}

// Screenlock - logged in, screen locked.
IN_PROC_BROWSER_TEST_F(AuthPrivateApiTest, ScreenLock) {
  ScreenLocker::Show();
  std::unique_ptr<test::ScreenLockerTester> tester(ScreenLocker::GetTester());
  tester->EmulateWindowManagerReady();
  content::WindowedNotificationObserver lock_state_observer(
      chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
      content::NotificationService::AllSources());
  if (!tester->IsLocked())
    lock_state_observer.Wait();
  EXPECT_EQ(session_manager::SessionState::LOCKED,
            session_manager::SessionManager::Get()->session_state());
  ASSERT_TRUE(RunComponentExtensionTest("auth_private/lock")) << message_;
}

}  // namespace chromeos
