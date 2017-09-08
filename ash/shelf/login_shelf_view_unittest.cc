// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include "ash/login/mock_lock_screen_client.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/shutdown_controller.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "ash/tray_action/test_tray_action_client.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_state_controller.h"
#include "ui/events/event_utils.h"
#include "ui/views/controls/button/label_button.h"

namespace ash {

#define EXPECT_SHELF_BUTTONS(buttons)                                     \
  EXPECT_EQ(static_cast<unsigned long>(login_shelf_view_->child_count()), \
            buttons.size());                                              \
  for (int i = 0; i < login_shelf_view_->child_count(); ++i)              \
    EXPECT_EQ(login_shelf_view_->child_at(i), buttons[i]);

class LoginShelfViewTest : public AshTestBase {
 public:
  LoginShelfViewTest() {}
  ~LoginShelfViewTest() override {}

  void SetUp() override {
    AshTestBase::SetUp();
    login_shelf_view_ = GetPrimaryShelf()->GetLoginShelfViewForTesting();
    shell_delegate_ =
        static_cast<TestShellDelegate*>(Shell::Get()->shell_delegate());
    Shell::Get()->tray_action()->SetClient(
        tray_action_client.CreateInterfacePtrAndBind(),
        mojom::TrayActionState::kNotAvailable);
    login_shelf_test_api_ =
        base::MakeUnique<LoginShelfView::TestApi>(login_shelf_view_);
    shutdown_test_api_ = base::MakeUnique<ShutdownController::TestApi>(
        Shell::Get()->shutdown_controller());
  }

 protected:
  // |login_shelf_view_| should not contain buttons under initial states.
  void SetInitialStates() {
    NotifySessionStateChanged(session_manager::SessionState::ACTIVE);
    NotifyShutdownPolicyChanged(false);
    NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  }

  void NotifySessionStateChanged(session_manager::SessionState state) {
    GetSessionControllerClient()->SetSessionState(state);
  }

  void NotifyShutdownPolicyChanged(bool reboot_on_shutdown) {
    shutdown_test_api_->SetRebootOnShutdown(reboot_on_shutdown);
  }

  void NotifyLockScreenNoteStateChanged(mojom::TrayActionState state) {
    Shell::Get()->tray_action()->UpdateLockScreenNoteState(state);
  }

  // Simulates a click event on the button.
  void Click(views::LabelButton* button) {
    const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                               ui::EventTimeForNow(), 0, 0);
    login_shelf_view_->ButtonPressed(button, event);
    RunAllPendingInMessageLoop();
  }

  TestTrayActionClient tray_action_client;

  std::unique_ptr<LoginShelfView::TestApi> login_shelf_test_api_;

  std::unique_ptr<ShutdownController::TestApi> shutdown_test_api_;

  LoginShelfView* login_shelf_view_;  // Unowned.

  TestShellDelegate* shell_delegate_;  // Unowned.

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginShelfViewTest);
};

// Checks the login shelf updates UI after session state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterSessionStateChange) {
  std::vector<views::LabelButton*> buttons;
  SetInitialStates();
  buttons = {};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::OOBE);
  buttons = {login_shelf_test_api_->shutdown_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_PRIMARY);
  buttons = {login_shelf_test_api_->shutdown_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_SECONDARY);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->cancel_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(
      session_manager::SessionState::LOGGED_IN_NOT_ACTIVE);
  buttons = {login_shelf_test_api_->shutdown_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::UNKNOWN);
  buttons = {};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::ACTIVE);
  buttons = {};
  EXPECT_SHELF_BUTTONS(buttons);
}

// Checks the login shelf updates UI after shutdown polich change when the
// screen is locked.
TEST_F(LoginShelfViewTest,
       ShouldUpdateUiAfterShutdownPolicyChangeAtLockScreen) {
  std::vector<views::LabelButton*> buttons;
  SetInitialStates();
  buttons = {};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  buttons = {login_shelf_test_api_->restart_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyShutdownPolicyChanged(false /*reboot_on_shutdown*/);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);
}

// Checks shutdown policy change during another session state (e.g. ACTIVE)
// will be reflected when the screen becomes locked.
TEST_F(LoginShelfViewTest, ShouldUpdateUiBasedOnShutdownPolicyInActiveSession) {
  std::vector<views::LabelButton*> buttons;
  // The initial state of |reboot_on_shutdown| is false.
  SetInitialStates();
  buttons = {};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {login_shelf_test_api_->restart_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);
}

// Checks the login shelf updates UI after lock screen note state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterLockScreenNoteState) {
  std::vector<views::LabelButton*> buttons;
  SetInitialStates();
  buttons = {};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kAvailable);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  buttons = {login_shelf_test_api_->close_note_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  buttons = {login_shelf_test_api_->close_note_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kBackground);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  buttons = {login_shelf_test_api_->shutdown_button(),
             login_shelf_test_api_->sign_out_button()};
  EXPECT_SHELF_BUTTONS(buttons);
}

TEST_F(LoginShelfViewTest, ClickShutdownButton) {
  Click(login_shelf_test_api_->shutdown_button());
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickRestartButton) {
  Click(login_shelf_test_api_->restart_button());
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickSignOutButton) {
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 0);
  Click(login_shelf_test_api_->sign_out_button());
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 1);
}

TEST_F(LoginShelfViewTest, ClickUnlockButton) {
  // The unlock button is visible only when session state is LOCKED and note
  // state is kActive or kLaunching.
  NotifySessionStateChanged(session_manager::SessionState::LOCKED);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_EQ(tray_action_client.action_close_count(), 0);
  Click(login_shelf_test_api_->close_note_button());
  EXPECT_EQ(tray_action_client.action_close_count(), 1);

  tray_action_client.ClearCounts();
  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_EQ(tray_action_client.action_close_count(), 0);
  Click(login_shelf_test_api_->close_note_button());
  EXPECT_EQ(tray_action_client.action_close_count(), 1);
}

TEST_F(LoginShelfViewTest, ClickCancelButton) {
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  EXPECT_CALL(*client, CancelAddUser());
  Click(login_shelf_test_api_->cancel_button());
}

}  // namespace ash
