// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/launcher.h"
#include "base/command_line.h"
#include "chrome/browser/chromeos/lock_screen_apps/state_controller.h"
#include "chrome/browser/chromeos/note_taking_helper.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "components/prefs/pref_service.h"
#include "components/session_manager/core/session_manager.h"
#include "extensions/common/api/app_runtime.h"
#include "extensions/common/switches.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"

namespace {

const char kTestAppId[] = "cadfeochfldmbdgoccgbeianhamecbae";

class LockScreenNoteTakingTest : public ExtensionBrowserTest {
 public:
  LockScreenNoteTakingTest() { set_chromeos_user_ = true; }
  ~LockScreenNoteTakingTest() override = default;

  void SetUpCommandLine(base::CommandLine* cmd_line) override {
    cmd_line->AppendSwitchASCII(extensions::switches::kWhitelistedExtensionID,
                                kTestAppId);
    cmd_line->AppendSwitch(chromeos::switches::kEnableLockScreenApps);

    ExtensionBrowserTest::SetUpCommandLine(cmd_line);
  }

  bool EnableLockScreenAppLaunch(const std::string& app_id) {
    chromeos::NoteTakingHelper::Get()->SetPreferredApp(profile(), app_id);
    profile()->GetPrefs()->SetBoolean(prefs::kNoteTakingAppEnabledOnLockScreen,
                                      true);
    session_manager::SessionManager::Get()->SetSessionState(
        session_manager::SessionState::LOCKED);

    return lock_screen_apps::StateController::Get()->GetLockScreenNoteState() ==
           ash::mojom::TrayActionState::kAvailable;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LockScreenNoteTakingTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(LockScreenNoteTakingTest, Launch) {
  ASSERT_TRUE(lock_screen_apps::StateController::IsEnabled());

  scoped_refptr<const extensions::Extension> app =
      LoadExtension(test_data_dir_.AppendASCII("lock_screen_apps/app_launch"));
  ASSERT_TRUE(EnableLockScreenAppLaunch(app->id()));

  // Message the test app will send from the created app window once the tests
  // are run and the window is ready to be closed.
  // The test should reply to this message in order for the app window to close
  // itself.
  ExtensionTestMessageListener ready_to_close("readyToClose",
                                              true /* will_reply */);

  extensions::ResultCatcher catcher;
  lock_screen_apps::StateController::Get()->RequestNewLockScreenNote();

  ASSERT_EQ(ash::mojom::TrayActionState::kLaunching,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  // The test will run two sets of tests:
  // * in window that gets created as the response to action launch
  // * in the app background page - the test will launch an app window and wait
  //   for it to be closed
  // Wait for both of those to finish.
  if (!catcher.GetNextResult())
    ADD_FAILURE() << catcher.message();

  ASSERT_EQ(ash::mojom::TrayActionState::kActive,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  ready_to_close.WaitUntilSatisfied();
  ready_to_close.Reply("close");

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  EXPECT_EQ(ash::mojom::TrayActionState::kAvailable,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());
}

IN_PROC_BROWSER_TEST_F(LockScreenNoteTakingTest, LaunchInNonLockScreenContext) {
  ASSERT_TRUE(lock_screen_apps::StateController::IsEnabled());

  scoped_refptr<const extensions::Extension> app = LoadExtension(
      test_data_dir_.AppendASCII("lock_screen_apps/non_lock_screen_context"));
  ASSERT_TRUE(EnableLockScreenAppLaunch(app->id()));

  extensions::ResultCatcher catcher;
  lock_screen_apps::StateController::Get()->RequestNewLockScreenNote();

  ASSERT_EQ(ash::mojom::TrayActionState::kLaunching,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  auto action_data =
      base::MakeUnique<extensions::api::app_runtime::ActionData>();
  action_data->action_type =
      extensions::api::app_runtime::ActionType::ACTION_TYPE_NEW_NOTE;
  apps::LaunchPlatformAppWithAction(profile(), app.get(),
                                    std::move(action_data), base::FilePath());

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_F(LockScreenNoteTakingTest, DataCreation) {
  ASSERT_TRUE(lock_screen_apps::StateController::IsEnabled());

  scoped_refptr<const extensions::Extension> app = LoadExtension(
      test_data_dir_.AppendASCII("lock_screen_apps/data_provider"));
  ASSERT_TRUE(EnableLockScreenAppLaunch(app->id()));

  // Message the test app will send from the created app window once the tests
  // are run and the window is ready to be closed.
  // The test should reply to this message in order for the app window to close
  // itself.
  ExtensionTestMessageListener ready_to_close("readyToClose",
                                              true /* will_reply */);

  extensions::ResultCatcher catcher;
  lock_screen_apps::StateController::Get()->RequestNewLockScreenNote();

  ASSERT_EQ(ash::mojom::TrayActionState::kLaunching,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  // The test will run two sets of tests:
  // * in window that gets created as the response to action launch
  // * in the app background page - the test will launch an app window and wait
  //   for it to be closed
  // Wait for both of those to finish.
  if (!catcher.GetNextResult())
    ADD_FAILURE() << catcher.message();

  ASSERT_EQ(ash::mojom::TrayActionState::kActive,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  ready_to_close.WaitUntilSatisfied();
  ready_to_close.Reply("close");

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  EXPECT_EQ(ash::mojom::TrayActionState::kAvailable,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  session_manager::SessionManager::Get()->SetSessionState(
      session_manager::SessionState::ACTIVE);

  // Unlocking the session should trigger onDataItemsAvailable event, which
  // should be catched by the background page in the main app - the event should
  // start another test sequence.
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_F(LockScreenNoteTakingTest, PRE_DataAvailableOnRestart) {
  ASSERT_TRUE(lock_screen_apps::StateController::IsEnabled());

  scoped_refptr<const extensions::Extension> app = LoadExtension(
      test_data_dir_.AppendASCII("lock_screen_apps/data_provider"));
  ASSERT_TRUE(EnableLockScreenAppLaunch(app->id()));

  // Message the test app will send from the created app window once the tests
  // are run and the window is ready to be closed.
  // The test should reply to this message in order for the app window to close
  // itself.
  ExtensionTestMessageListener ready_to_close("readyToClose",
                                              true /* will_reply */);

  extensions::ResultCatcher catcher;
  lock_screen_apps::StateController::Get()->RequestNewLockScreenNote();

  ASSERT_EQ(ash::mojom::TrayActionState::kLaunching,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  // The test will run two sets of tests:
  // * in window that gets created as the response to action launch
  // * in the app background page - the test will launch an app window and wait
  //   for it to be closed
  // Wait for both of those to finish.
  if (!catcher.GetNextResult())
    ADD_FAILURE() << catcher.message();

  ASSERT_EQ(ash::mojom::TrayActionState::kActive,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());

  ready_to_close.WaitUntilSatisfied();
  ready_to_close.Reply("close");

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  EXPECT_EQ(ash::mojom::TrayActionState::kAvailable,
            lock_screen_apps::StateController::Get()->GetLockScreenNoteState());
}

IN_PROC_BROWSER_TEST_F(LockScreenNoteTakingTest, DataAvailableOnRestart) {
  // In PRE_ part  of the test there were data items created in the lock screen
  // storage - when the lock screen note taking is initialized,
  // OnDataItemsAvailable should be dispatched to the test app (given that the
  // lock screen app's data storage is non empty), which should in turn run a
  // sequence of API tests (in the test app background page).
  // This test is intended to catch the result if these tests.
  extensions::ResultCatcher catcher;
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
}
