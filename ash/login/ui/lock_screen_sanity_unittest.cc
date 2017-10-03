// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/lock_screen_controller.h"
#include "ash/login/mock_lock_screen_client.h"
#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/login/ui/login_test_utils.h"
#include "ash/public/cpp/config.h"
#include "ash/root_window_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"

using ::testing::_;
using ::testing::Invoke;
using LockScreenSanityTest = ash::LoginTestBase;

namespace ash {
namespace {

class LockScreenAppFocuser {
 public:
  explicit LockScreenAppFocuser(views::Widget* lock_screen_app_widget)
      : lock_screen_app_widget_(lock_screen_app_widget) {}
  ~LockScreenAppFocuser() = default;

  bool reversed_tab_order() const { return reversed_tab_order_; }

  void FocusLockScreenApp(bool reverse) {
    reversed_tab_order_ = reverse;
    lock_screen_app_widget_->Activate();
  }

 private:
  bool reversed_tab_order_ = false;
  views::Widget* lock_screen_app_widget_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenAppFocuser);
};

// Returns true if |view| or any child of it has focus.
bool HasFocusInAnyChildView(views::View* view) {
  if (view->HasFocus())
    return true;
  for (int i = 0; i < view->child_count(); ++i) {
    if (HasFocusInAnyChildView(view->child_at(i)))
      return true;
  }
  return false;
}

// Keeps tabbing through |view| until the view loses focus.
// The number of generated tab events will be limited - if the focus is still
// within the view by the time the limit is hit, this will return false.
bool TabThroughView(ui::test::EventGenerator* event_generator,
                    views::View* view,
                    bool reverse) {
  if (!HasFocusInAnyChildView(view)) {
    ADD_FAILURE() << "View not focused initially.";
    return false;
  }

  for (int i = 0; i < 50; ++i) {
    event_generator->PressKey(ui::KeyboardCode::VKEY_TAB,
                              reverse ? ui::EF_SHIFT_DOWN : 0);
    if (!HasFocusInAnyChildView(view))
      return true;
  }

  return false;
}

void ExpectFocused(views::View* view) {
  EXPECT_TRUE(view->GetWidget()->IsActive());
  EXPECT_TRUE(HasFocusInAnyChildView(view));
}

void ExpectNotFocused(views::View* view) {
  EXPECT_FALSE(view->GetWidget()->IsActive());
  EXPECT_FALSE(HasFocusInAnyChildView(view));
}

}  // namespace

// Verifies that the password input box has focus.
TEST_F(LockScreenSanityTest, PasswordIsInitiallyFocused) {
  // Build lock screen.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher());

  // The lock screen requires at least one user.
  SetUserCount(1);

  ShowWidgetWithContent(contents);

  // Textfield should have focus.
  EXPECT_EQ(MakeLoginPasswordTestApi(contents).textfield(),
            contents->GetFocusManager()->GetFocusedView());
}

// Verifies submitting the password invokes mojo lock screen client.
TEST_F(LockScreenSanityTest, PasswordSubmitCallsLockScreenClient) {
  // TODO: Renable in mash once crbug.com/725257 is fixed.
  if (Shell::GetAshConfig() == Config::MASH)
    return;

  // Build lock screen.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher());

  // The lock screen requires at least one user.
  SetUserCount(1);

  ShowWidgetWithContent(contents);

  // Password submit runs mojo.
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  client->set_authenticate_user_callback_result(false);
  EXPECT_CALL(*client, AuthenticateUser_(users()[0]->account_id, _, false, _));
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  base::RunLoop().RunUntilIdle();
}

// Verifies that tabbing from the lock screen will eventually focus the shelf.
// Then, a shift+tab will bring focus back to the lock screen.
TEST_F(LockScreenSanityTest, TabGoesFromLockToShelfAndBackToLock) {
  // Make lock screen shelf visible.
  GetSessionControllerClient()->SetSessionState(
      session_manager::SessionState::LOCKED);

  // Create lock screen.
  auto* lock = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                    data_dispatcher());
  SetUserCount(1);
  ShowWidgetWithContent(lock);
  views::View* shelf = Shelf::ForWindow(lock->GetWidget()->GetNativeWindow())
                           ->shelf_widget()
                           ->GetContentsView();

  // Lock has focus.
  ExpectFocused(lock);
  ExpectNotFocused(shelf);

  // Tab (eventually) goes to the shelf.
  ASSERT_TRUE(TabThroughView(&GetEventGenerator(), lock, false /*reverse*/));
  ExpectNotFocused(lock);
  ExpectFocused(shelf);

  // A single shift+tab brings focus back to the lock screen.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, ui::EF_SHIFT_DOWN);
  ExpectFocused(lock);
  ExpectNotFocused(shelf);
}

// Verifies that shift-tabbing from the lock screen will eventually focus the
// status area. Then, a tab will bring focus back to the lock screen.
TEST_F(LockScreenSanityTest, ShiftTabGoesFromLockToStatusAreaAndBackToLock) {
  // The status area will not focus out unless we are on the lock screen. See
  // StatusAreaWidgetDelegate::ShouldFocusOut.
  GetSessionControllerClient()->SetSessionState(
      session_manager::SessionState::LOCKED);

  auto* lock = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                    data_dispatcher());
  SetUserCount(1);
  ShowWidgetWithContent(lock);
  views::View* status_area =
      RootWindowController::ForWindow(lock->GetWidget()->GetNativeWindow())
          ->GetSystemTray()
          ->GetWidget()
          ->GetContentsView();

  // Lock screen has focus.
  ExpectFocused(lock);
  ExpectNotFocused(status_area);

  // Two shift+tab bring focus to the status area.
  // TODO(crbug.com/768076): Only one shift+tab is needed as the focus should
  // go directly to status area from password view.
  //
  // Focus from password view to user view (user dropdown button).
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, ui::EF_SHIFT_DOWN);

  // Focus from user view to the status area.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, ui::EF_SHIFT_DOWN);

  ExpectNotFocused(lock);
  ExpectFocused(status_area);

  // A single tab brings focus back to the lock screen.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, 0);
  ExpectFocused(lock);
  ExpectNotFocused(status_area);
}

TEST_F(LockScreenSanityTest, TabWithLockScreenAppActive) {
  GetSessionControllerClient()->SetSessionState(
      session_manager::SessionState::LOCKED);

  auto* lock = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                    data_dispatcher());
  SetUserCount(1);
  ShowWidgetWithContent(lock);

  views::View* shelf = Shelf::ForWindow(lock->GetWidget()->GetNativeWindow())
                           ->shelf_widget()
                           ->GetContentsView();

  views::View* status_area =
      RootWindowController::ForWindow(lock->GetWidget()->GetNativeWindow())
          ->GetSystemTray()
          ->GetWidget()
          ->GetContentsView();

  // Initialize lock screen action state.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kActive);

  // Create and focus a lock screen app window.
  auto* lock_screen_app = new views::View();
  lock_screen_app->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  views::Widget* app_widget = CreateWidgetWithContent(lock_screen_app);
  app_widget->Show();

  // Lock screen app focus is requested using lock screen mojo client - set up
  // the mock client.
  LockScreenAppFocuser app_widget_focuser(app_widget);
  std::unique_ptr<MockLockScreenClient> client = BindMockLockScreenClient();
  EXPECT_CALL(*client, FocusLockScreenApps(_))
      .WillRepeatedly(Invoke(&app_widget_focuser,
                             &LockScreenAppFocuser::FocusLockScreenApp));

  // Initially, focus should be with the lock screen app - when the app loses
  // focus (notified via mojo interface), shelf should get the focus next.
  ExpectFocused(lock_screen_app);
  Shell::Get()->lock_screen_controller()->HandleFocusLeavingLockScreenApps(
      false /*reverse*/);
  ExpectFocused(shelf);

  // Reversing focus should bring focus back to the lock screen app.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, ui::EF_SHIFT_DOWN);
  // Focus should be handled off asynchronously - run loop for focus change to
  // propagate (through mojo).
  base::RunLoop().RunUntilIdle();
  ExpectFocused(lock_screen_app);
  EXPECT_TRUE(app_widget_focuser.reversed_tab_order());

  // Have the app tab out in reverse tab order - in this case, the status area
  // should get the focus.
  Shell::Get()->lock_screen_controller()->HandleFocusLeavingLockScreenApps(
      true /*reverse*/);
  ExpectFocused(status_area);

  // Tabbing out of the status area (in default order) should focus the lock
  // screen app again.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, 0);
  base::RunLoop().RunUntilIdle();
  ExpectFocused(lock_screen_app);
  EXPECT_FALSE(app_widget_focuser.reversed_tab_order());

  // Tab out of the lock screen app once more - the shelf should get the focus
  // again.
  Shell::Get()->lock_screen_controller()->HandleFocusLeavingLockScreenApps(
      false /*reverse*/);
  ExpectFocused(shelf);

  app_widget->Close();
}

TEST_F(LockScreenSanityTest, FocusLockScreenWhenLockScreenAppExit) {
  // Set up lock screen.
  GetSessionControllerClient()->SetSessionState(
      session_manager::SessionState::LOCKED);
  auto* lock = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                    data_dispatcher());
  SetUserCount(1);
  ShowWidgetWithContent(lock);

  views::View* shelf = Shelf::ForWindow(lock->GetWidget()->GetNativeWindow())
                           ->shelf_widget()
                           ->GetContentsView();

  // Setup and focus a lock screen app.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kActive);
  auto* lock_screen_app = new views::View();
  lock_screen_app->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  views::Widget* app_widget = CreateWidgetWithContent(lock_screen_app);
  app_widget->Show();
  ExpectFocused(lock_screen_app);

  // Tab out of the lock screen app - shelf should get the focus.
  Shell::Get()->lock_screen_controller()->HandleFocusLeavingLockScreenApps(
      false /*reverse*/);
  ExpectFocused(shelf);

  // Move the lock screen note taking to available state (which happens when the
  // app session ends) - this should focus the lock screen.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kAvailable);
  ExpectFocused(lock);

  // Tab through the lock screen - the focus should eventually get to the shelf.
  ASSERT_TRUE(TabThroughView(&GetEventGenerator(), lock, false /*reverse*/));
  ExpectFocused(shelf);

  app_widget->Close();
}

}  // namespace ash
