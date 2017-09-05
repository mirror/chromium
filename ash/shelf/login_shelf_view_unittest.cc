// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include "ash/login/mock_lock_screen_client.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "ash/tray_action/test_tray_action_client.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_state_controller_test_api.h"
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
    lock_state_test_api_ = base::MakeUnique<LockStateControllerTestApi>(
        Shell::Get()->lock_state_controller());
    TrayAction* tray_action = Shell::Get()->tray_action();
    tray_action->SetClient(tray_action_client.CreateInterfacePtrAndBind(),
                           mojom::TrayActionState::kNotAvailable);
  }

 protected:
  // The login_shelf_view should not contain buttons under initial states.
  void SetInitialStates() {
    NotifySessionStateChanged(session_manager::SessionState::ACTIVE);
    NotifyShutdownPolicyChanged(false);
    NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  }

  void NotifySessionStateChanged(session_manager::SessionState state) {
    GetSessionControllerClient()->SetSessionState(state);
  }

  void NotifyShutdownPolicyChanged(bool reboot_on_shutdown) {
    login_shelf_view_->UpdateShutdownPolicy(reboot_on_shutdown);
  }

  void NotifyLockScreenNoteStateChanged(mojom::TrayActionState state) {
    Shell::Get()->tray_action()->UpdateLockScreenNoteState(state);
  }

  // Simulates a click event on the button of the particular type.
  void ClickOnButton(LoginShelfView::ButtonType type) {
    auto it = login_shelf_view_->type_to_button.find(type);
    DCHECK(it != login_shelf_view_->type_to_button.end());
    views::LabelButton* button = it->second;

    const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                               ui::EventTimeForNow(), 0, 0);
    login_shelf_view_->ButtonPressed(button, event);
  }

  bool VerifyButtonType(views::View* button, LoginShelfView::ButtonType type) {
    auto it = login_shelf_view_->button_to_type.find(
        static_cast<views::LabelButton*>(button));
    DCHECK(it != login_shelf_view_->button_to_type.end());
    return it->second == type;
  }

  LoginShelfView* login_shelf_view_ = nullptr;

  std::unique_ptr<LockStateControllerTestApi> lock_state_test_api_;

  TestTrayActionClient tray_action_client;

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
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_PRIMARY);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_SECONDARY);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kCancel));

  NotifySessionStateChanged(
      session_manager::SessionState::LOGGED_IN_NOT_ACTIVE);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));

  NotifySessionStateChanged(session_manager::SessionState::UNKNOWN);
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));

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
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kRestart));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));

  NotifyShutdownPolicyChanged(false /*reboot_on_shutdown*/);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));
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
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kRestart));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));
}

// Checks the login shelf updates UI after lock screen note state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterLockScreenNoteState) {
  SetInitialStates();
  EXPECT_FALSE(login_shelf_view_->has_children());

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kAvailable);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kUnlock));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_EQ(login_shelf_view_->child_count(), 1);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kUnlock));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kBackground);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  EXPECT_EQ(login_shelf_view_->child_count(), 2);
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(0),
                               LoginShelfView::ButtonType::kShutdown));
  EXPECT_TRUE(VerifyButtonType(login_shelf_view_->child_at(1),
                               LoginShelfView::ButtonType::kSignOut));
}

TEST_F(LoginShelfViewTest, ClickShutdownButton) {
  ClickOnButton(LoginShelfView::ButtonType::kShutdown);
  EXPECT_TRUE(lock_state_test_api_->real_shutdown_timer_is_running());
}

TEST_F(LoginShelfViewTest, ClickRestartButton) {
  ClickOnButton(LoginShelfView::ButtonType::kRestart);
  EXPECT_TRUE(lock_state_test_api_->real_shutdown_timer_is_running());
}

TEST_F(LoginShelfViewTest, ClickSignOutButton) {
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 0);
  ClickOnButton(LoginShelfView::ButtonType::kSignOut);
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 1);
}

TEST_F(LoginShelfViewTest, ClickUnlockButton) {
  // The unlock button is visible only when session state is LOCKED.
  NotifySessionStateChanged(session_manager::SessionState::LOCKED);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_CALL(tray_action_client, ToggleForegroundMode(true));
  ClickOnButton(LoginShelfView::ButtonType::kUnlock);
  base::RunLoop().RunUntilIdle();

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_CALL(tray_action_client, ToggleForegroundMode(true));
  ClickOnButton(LoginShelfView::ButtonType::kUnlock);
  base::RunLoop().RunUntilIdle();

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kBackground);
  EXPECT_CALL(tray_action_client, ToggleForegroundMode(false));
  ClickOnButton(LoginShelfView::ButtonType::kUnlock);
  base::RunLoop().RunUntilIdle();
}

TEST_F(LoginShelfViewTest, CancelButton) {
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  EXPECT_CALL(*client, CancelAddUser());
  ClickOnButton(LoginShelfView::ButtonType::kCancel);
  base::RunLoop().RunUntilIdle();
}

}  // namespace ash
