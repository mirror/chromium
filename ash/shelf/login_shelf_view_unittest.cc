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
namespace {

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
  void Click(LoginShelfView::ButtonId id) {
    const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                               ui::EventTimeForNow(), 0, 0);
    login_shelf_view_->ButtonPressed(
        static_cast<views::Button*>(login_shelf_view_->GetViewByID(id)), event);
    RunAllPendingInMessageLoop();
  }

  // Checks if the shelf is only showing the buttons in the list. The IDs in
  // the specified list must be unique.
  bool ShowsShelfButtons(std::vector<LoginShelfView::ButtonId> id_list) {
    for (LoginShelfView::ButtonId id : id_list) {
      if (!login_shelf_view_->GetViewByID(id)->visible())
        return false;
    }
    size_t visible_button_count = 0;
    for (int i = 0; i < login_shelf_view_->child_count(); ++i) {
      if (login_shelf_view_->child_at(i)->visible())
        visible_button_count++;
    }
    return visible_button_count == id_list.size();
  }

  TestTrayActionClient tray_action_client;

  std::unique_ptr<ShutdownController::TestApi> shutdown_test_api_;

  LoginShelfView* login_shelf_view_;  // Unowned.

  TestShellDelegate* shell_delegate_;  // Unowned.

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginShelfViewTest);
};

// Checks the login shelf updates UI after session state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterSessionStateChange) {
  std::vector<LoginShelfView::ButtonId> buttons;
  SetInitialStates();
  buttons = {};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::OOBE);
  buttons = {LoginShelfView::kShutdown};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_PRIMARY);
  buttons = {LoginShelfView::kShutdown};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::LOGIN_SECONDARY);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kCancel};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(
      session_manager::SessionState::LOGGED_IN_NOT_ACTIVE);
  buttons = {LoginShelfView::kShutdown};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::UNKNOWN);
  buttons = {};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::ACTIVE);
  buttons = {};
  EXPECT_TRUE(ShowsShelfButtons(buttons));
}

// Checks the login shelf updates UI after shutdown polich change when the
// screen is locked.
TEST_F(LoginShelfViewTest,
       ShouldUpdateUiAfterShutdownPolicyChangeAtLockScreen) {
  std::vector<LoginShelfView::ButtonId> buttons;
  SetInitialStates();
  buttons = {};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  buttons = {LoginShelfView::kRestart, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyShutdownPolicyChanged(false /*reboot_on_shutdown*/);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));
}

// Checks shutdown policy change during another session state (e.g. ACTIVE)
// will be reflected when the screen becomes locked.
TEST_F(LoginShelfViewTest, ShouldUpdateUiBasedOnShutdownPolicyInActiveSession) {
  std::vector<LoginShelfView::ButtonId> buttons;
  // The initial state of |reboot_on_shutdown| is false.
  SetInitialStates();
  buttons = {};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {LoginShelfView::kRestart, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));
}

// Checks the login shelf updates UI after lock screen note state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterLockScreenNoteState) {
  std::vector<LoginShelfView::ButtonId> buttons;
  SetInitialStates();
  buttons = {};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifySessionStateChanged(session_manager::SessionState::LOCKED);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kAvailable);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  buttons = {LoginShelfView::kCloseNote};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  buttons = {LoginShelfView::kCloseNote};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kBackground);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  buttons = {LoginShelfView::kShutdown, LoginShelfView::kSignOut};
  EXPECT_TRUE(ShowsShelfButtons(buttons));
}

TEST_F(LoginShelfViewTest, ClickShutdownButton) {
  Click(LoginShelfView::kShutdown);
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickRestartButton) {
  Click(LoginShelfView::kRestart);
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickSignOutButton) {
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 0);
  Click(LoginShelfView::kSignOut);
  EXPECT_EQ(shell_delegate_->num_exit_requests(), 1);
}

TEST_F(LoginShelfViewTest, ClickUnlockButton) {
  // The unlock button is visible only when session state is LOCKED and note
  // state is kActive or kLaunching.
  NotifySessionStateChanged(session_manager::SessionState::LOCKED);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_EQ(tray_action_client.action_close_count(), 0);
  Click(LoginShelfView::kCloseNote);
  EXPECT_EQ(tray_action_client.action_close_count(), 1);

  tray_action_client.ClearCounts();
  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_EQ(tray_action_client.action_close_count(), 0);
  Click(LoginShelfView::kCloseNote);
  EXPECT_EQ(tray_action_client.action_close_count(), 1);
}

TEST_F(LoginShelfViewTest, ClickCancelButton) {
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  EXPECT_CALL(*client, CancelAddUser());
  Click(LoginShelfView::kCancel);
}

}  // namespace
}  // namespace ash
