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
  void ClickOnButton(views::LabelButton* button) {
    const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                               ui::EventTimeForNow(), 0, 0);
    login_shelf_view_->ButtonPressed(button, event);
    RunAllPendingInMessageLoop();
  }

  // Convenience functions to get the buttons.
  views::LabelButton* shutdown_button() {
    return login_shelf_view_->shutdown_button_;
  }

  views::LabelButton* restart_button() {
    return login_shelf_view_->restart_button_;
  }

  views::LabelButton* sign_out_button() {
    return login_shelf_view_->sign_out_button_;
  }

  views::LabelButton* unlock_button() {
    return login_shelf_view_->unlock_button_;
  }

  views::LabelButton* cancel_button() {
    return login_shelf_view_->cancel_button_;
  }

  LoginShelfView* login_shelf_view_ = nullptr;

  TestTrayActionClient tray_action_client;

  std::unique_ptr<ShutdownController::TestApi> shutdown_test_api_;

  TestShellDelegate* shell_delegate_;  // not owned

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginShelfViewTest);
};

// Checks the login shelf updates UI after session state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterSessionStateChange) {
  SetInitialStates();
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::OOBE);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_PRIMARY);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_SECONDARY);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), cancel_button());

  NotifySessionStateChanged(
      session_manager::SessionState::LOGGED_IN_NOT_ACTIVE);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());

  NotifySessionStateChanged(session_manager::SessionState::UNKNOWN);
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());

  NotifySessionStateChanged(session_manager::SessionState::ACTIVE);
  EXPECT_FALSE(login_shelf_view_->has_children());
}

// Checks the login shelf updates UI after shutdown polich change when the
// screen is locked.
TEST_F(LoginShelfViewTest,
       ShouldUpdateUiAfterShutdownPolicyChangeAtLockScreen) {
  SetInitialStates();
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), restart_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());

  NotifyShutdownPolicyChanged(false /*reboot_on_shutdown*/);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());
}

// Checks shutdown policy change during another session state (e.g. ACTIVE)
// will be reflected when the screen becomes locked.
TEST_F(LoginShelfViewTest, ShouldUpdateUiBasedOnShutdownPolicyInActiveSession) {
  // The initial state of |reboot_on_shutdown| is false.
  SetInitialStates();
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), restart_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());
}

// Checks the login shelf updates UI after lock screen note state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterLockScreenNoteState) {
  SetInitialStates();
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kAvailable);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_EQ(login_shelf_view_->child_at(0), unlock_button());

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_EQ(login_shelf_view_->child_at(0), unlock_button());

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kBackground);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_EQ(login_shelf_view_->child_at(0), shutdown_button());
  EXPECT_EQ(login_shelf_view_->child_at(1), sign_out_button());
}

TEST_F(LoginShelfViewTest, ClickShutdownButton) {
  ClickOnButton(shutdown_button());
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickRestartButton) {
  ClickOnButton(restart_button());
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickSignOutButton) {
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 0);
  ClickOnButton(sign_out_button());
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 1);
}

TEST_F(LoginShelfViewTest, ClickUnlockButton) {
  // The unlock button is visible only when session state is LOCKED and note
  // state is kActive or kLaunching.
  NotifySessionStateChanged(session_manager::SessionState::LOCKED);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_EQ(tray_action_client.action_close_count(), 0);
  ClickOnButton(unlock_button());
  EXPECT_EQ(tray_action_client.action_close_count(), 1);

  tray_action_client.ClearCounts();
  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_EQ(tray_action_client.action_close_count(), 0);
  ClickOnButton(unlock_button());
  EXPECT_EQ(tray_action_client.action_close_count(), 1);
}

TEST_F(LoginShelfViewTest, ClickCancelButton) {
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  EXPECT_CALL(*client, CancelAddUser());
  ClickOnButton(cancel_button());
}

}  // namespace ash
