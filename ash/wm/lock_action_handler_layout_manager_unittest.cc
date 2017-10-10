// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/lock_action_handler_layout_manager.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shell.h"
#include "ash/system/lock_screen_action/lock_screen_action_background_controller.h"
#include "ash/test/ash_test_base.h"
#include "ash/tray_action/test_tray_action_client.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/window_state.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_test_util.h"
#include "ui/keyboard/keyboard_ui.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

namespace {

constexpr int kVirtualKeyboardHeight = 100;

aura::Window* GetContainer(ShellWindowId container_id) {
  return Shell::GetPrimaryRootWindowController()->GetContainer(container_id);
}

LockActionHandlerLayoutManager* GetLockActionHandlerLayoutManager() {
  return static_cast<LockActionHandlerLayoutManager*>(
      GetContainer(kShellWindowId_LockActionHandlerContainer)
          ->layout_manager());
}

class TestWindowDelegate : public views::WidgetDelegate {
 public:
  explicit TestWindowDelegate(bool can_activate)
      : can_activate_(can_activate) {}
  ~TestWindowDelegate() override = default;

  // views::WidgetDelegate:
  void DeleteDelegate() override { delete this; }
  views::Widget* GetWidget() override { return widget_; }
  const views::Widget* GetWidget() const override { return widget_; }
  bool CanActivate() const override { return can_activate_; }
  bool CanResize() const override { return true; }
  bool CanMaximize() const override { return true; }
  bool ShouldAdvanceFocusToTopLevelWidget() const override { return true; }

  void set_widget(views::Widget* widget) { widget_ = widget; }

 private:
  views::Widget* widget_ = nullptr;
  bool can_activate_;

  DISALLOW_COPY_AND_ASSIGN(TestWindowDelegate);
};

std::unique_ptr<aura::Window> CreateTestingWindow(
    views::Widget::InitParams params,
    ShellWindowId parent_id,
    std::unique_ptr<TestWindowDelegate> window_delegate) {
  params.parent = GetContainer(parent_id);
  views::Widget* widget = new views::Widget;
  if (window_delegate) {
    window_delegate->set_widget(widget);
    params.delegate = window_delegate.release();
  }
  widget->Init(params);
  widget->Show();
  return base::WrapUnique<aura::Window>(widget->GetNativeView());
}

class TestBackgroundController : public LockScreenActionBackgroundController {
 public:
  TestBackgroundController() {}
  ~TestBackgroundController() override {}

  // LockScreenBackgroundController:
  bool IsBackgroundWindow(aura::Window* window) const override {
    return window_ && window_ == window;
  }

  bool ShowBackground() override {
    if (state() == LockScreenActionBackgroundState::kShowing ||
        state() == LockScreenActionBackgroundState::kShown) {
      return false;
    }

    if (!window_) {
      window_ =
          CreateTestingWindow(
              views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
              kShellWindowId_LockScreenContainer,
              std::make_unique<TestWindowDelegate>(false /*can_activate*/))
              .release();
    }
    window_->Show();

    UpdateState(LockScreenActionBackgroundState::kShowing);
    return true;
  }

  bool HideBackgroundImmediately() override {
    if (state() == LockScreenActionBackgroundState::kHidden)
      return false;

    window_->Hide();
    UpdateState(LockScreenActionBackgroundState::kHidden);
    return true;
  }

  bool HideBackground() override {
    if (state() == LockScreenActionBackgroundState::kHiding ||
        state() == LockScreenActionBackgroundState::kHidden) {
      return false;
    }

    UpdateState(LockScreenActionBackgroundState::kHiding);
    return true;
  }

  bool FinishShow() {
    if (state() != LockScreenActionBackgroundState::kShowing)
      return false;
    UpdateState(LockScreenActionBackgroundState::kShown);
    return true;
  }

  bool FinishHide() {
    if (state() != LockScreenActionBackgroundState::kHiding)
      return false;
    window_->Hide();
    UpdateState(LockScreenActionBackgroundState::kHidden);
    return true;
  }

  const aura::Window* window() const { return window_; }

 private:
  aura::Window* window_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TestBackgroundController);
};

}  // namespace

class LockActionHandlerLayoutManagerTest : public AshTestBase {
 public:
  LockActionHandlerLayoutManagerTest() = default;
  ~LockActionHandlerLayoutManagerTest() override = default;

  void SetUp() override {
    // Allow a virtual keyboard (and initialize it per default).
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        keyboard::switches::kEnableVirtualKeyboard);

    AshTestBase::SetUp();

    views::Widget::InitParams widget_params(
        views::Widget::InitParams::TYPE_WINDOW);
    widget_params.show_state = ui::SHOW_STATE_FULLSCREEN;
    lock_window_ = CreateTestingWindow(
        widget_params, kShellWindowId_LockScreenContainer,
        std::make_unique<TestWindowDelegate>(true /*can_activate*/));
  }

  void TearDown() override {
    Shell::GetPrimaryRootWindowController()->DeactivateKeyboard(
        keyboard::KeyboardController::GetInstance());
    lock_window_.reset();
    AshTestBase::TearDown();
  }

  // Show or hide the keyboard.
  void ShowKeyboard(bool show) {
    keyboard::KeyboardController* keyboard =
        keyboard::KeyboardController::GetInstance();
    ASSERT_TRUE(keyboard);
    if (show == keyboard->keyboard_visible())
      return;

    if (show) {
      keyboard->ShowKeyboard(true);
      keyboard->ui()->GetContentsWindow()->SetBounds(
          keyboard::KeyboardBoundsFromRootBounds(
              Shell::GetPrimaryRootWindow()->bounds(), kVirtualKeyboardHeight));
    } else {
      keyboard->HideKeyboard(keyboard::KeyboardController::HIDE_REASON_MANUAL);
    }

    DCHECK_EQ(show, keyboard->keyboard_visible());
  }

  void SetUpTrayActionClientAndLockSession(mojom::TrayActionState state) {
    Shell::Get()->tray_action()->SetClient(
        tray_action_client_.CreateInterfacePtrAndBind(), state);
    GetSessionControllerClient()->SetSessionState(
        session_manager::SessionState::LOCKED);
  }

 private:
  std::unique_ptr<aura::Window> lock_window_;

  TestTrayActionClient tray_action_client_;

  DISALLOW_COPY_AND_ASSIGN(LockActionHandlerLayoutManagerTest);
};

class LockActionHandlerLayoutManagerWithBackgroundTest
    : public LockActionHandlerLayoutManagerTest {
 public:
  LockActionHandlerLayoutManagerWithBackgroundTest() = default;
  ~LockActionHandlerLayoutManagerWithBackgroundTest() override = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch("show-md-login");
    LockActionHandlerLayoutManagerTest::SetUp();
    SetUpBackgroundController();
  }

  TestBackgroundController* background_controller() {
    return background_controller_.get();
  }

 private:
  void SetUpBackgroundController() {
    background_controller_ = std::make_unique<TestBackgroundController>();
    GetLockActionHandlerLayoutManager()->SetBackgroundControllerForTesting(
        background_controller_.get());
  }

  std::unique_ptr<TestBackgroundController> background_controller_;

  DISALLOW_COPY_AND_ASSIGN(LockActionHandlerLayoutManagerWithBackgroundTest);
};

TEST_F(LockActionHandlerLayoutManagerTest, PreserveNormalWindowBounds) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  const gfx::Rect bounds = gfx::Rect(10, 10, 300, 300);
  widget_params.bounds = bounds;
  // Note: default window delegate (used when no widget delegate is set) does
  // not allow the window to be maximized.
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      nullptr /* window_delegate */);
  EXPECT_EQ(bounds.ToString(), window->GetBoundsInScreen().ToString());

  gfx::Rect work_area =
      ScreenUtil::GetDisplayWorkAreaBoundsInParent(window.get());
  window->SetBounds(work_area);

  EXPECT_EQ(work_area.ToString(), window->GetBoundsInScreen().ToString());

  const gfx::Rect bounds2 = gfx::Rect(100, 100, 200, 200);
  window->SetBounds(bounds2);
  EXPECT_EQ(bounds2.ToString(), window->GetBoundsInScreen().ToString());
}

TEST_F(LockActionHandlerLayoutManagerTest, MaximizedWindowBounds) {
  // Cange the shelf alignment before locking the session.
  GetPrimaryShelf()->SetAlignment(SHELF_ALIGNMENT_RIGHT);

  // This should change the shelf alignment to bottom (temporarily for locked
  // state).
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_MAXIMIZED;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  // Verify that the window bounds are equal to work area for the bottom shelf
  // alignment, which matches how the shelf is aligned on the lock screen,
  gfx::Rect target_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      kShelfSize /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());
}

TEST_F(LockActionHandlerLayoutManagerTest, FullscreenWindowBounds) {
  // Cange the shelf alignment before locking the session.
  GetPrimaryShelf()->SetAlignment(SHELF_ALIGNMENT_RIGHT);

  // This should change the shelf alignment to bottom (temporarily for locked
  // state).
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_FULLSCREEN;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  // Verify that the window bounds are equal to work area for the bottom shelf
  // alignment, which matches how the shelf is aligned on the lock screen,
  gfx::Rect target_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      kShelfSize /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());
}

TEST_F(LockActionHandlerLayoutManagerTest, MaximizeResizableWindow) {
  GetSessionControllerClient()->SetSessionState(
      session_manager::SessionState::LOCKED);

  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  gfx::Rect target_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      kShelfSize /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());
}

TEST_F(LockActionHandlerLayoutManagerTest, KeyboardBounds) {
  gfx::Rect initial_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  initial_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                       kShelfSize /* bottom */);

  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_MAXIMIZED;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));
  ASSERT_EQ(initial_bounds.ToString(), window->GetBoundsInScreen().ToString());

  ShowKeyboard(true);

  gfx::Rect keyboard_bounds =
      keyboard::KeyboardController::GetInstance()->current_keyboard_bounds();
  // Sanity check that the keyboard intersects woth original window bounds - if
  // this is not true, the window bounds would remain unchanged.
  ASSERT_TRUE(keyboard_bounds.Intersects(initial_bounds));

  gfx::Rect target_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      keyboard_bounds.height() /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  // Verify that window bounds get updated when Chromevox bounds are shown (so
  // the Chromevox panel does not overlay with the action handler window).
  ash::ShelfLayoutManager* shelf_layout_manager =
      GetPrimaryShelf()->shelf_layout_manager();
  ASSERT_TRUE(shelf_layout_manager);

  const int chromevox_panel_height = 45;
  shelf_layout_manager->SetChromeVoxPanelHeight(chromevox_panel_height);

  target_bounds.Inset(0 /* left */, chromevox_panel_height /* top */,
                      0 /* right */, 0 /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  ShowKeyboard(false);
}

TEST_F(LockActionHandlerLayoutManagerTest, AddingWindowInActiveState) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_MAXIMIZED;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      nullptr /* window_delegate */);

  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(window->HasFocus());
}

TEST_F(LockActionHandlerLayoutManagerTest, AddingWindowInNonActiveState) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_MAXIMIZED;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      nullptr /* window_delegate */);

  // The window should not be visible if the note action is not in active state.
  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(window->HasFocus());

  // When the action state changes back to active, the window should be
  // shown.
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);
  EXPECT_EQ(GetContainer(kShellWindowId_LockActionHandlerContainer),
            window->parent());
  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(window->HasFocus());

  // The window should be hidden again upon leaving the active state.
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kAvailable);
  EXPECT_EQ(GetContainer(kShellWindowId_LockActionHandlerContainer),
            window->parent());
  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(window->HasFocus());
}

TEST_F(LockActionHandlerLayoutManagerTest, FocusWindowWhileInNonActiveState) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_MAXIMIZED;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      nullptr /* window_delegate */);

  EXPECT_EQ(GetContainer(kShellWindowId_LockActionHandlerContainer),
            window->parent());
  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(window->HasFocus());

  window->Focus();

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(window->HasFocus());
}

TEST_F(LockActionHandlerLayoutManagerTest,
       RequestShowWindowOutsideActiveState) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_MAXIMIZED;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      nullptr /* window_delegate */);

  EXPECT_EQ(GetContainer(kShellWindowId_LockActionHandlerContainer),
            window->parent());
  EXPECT_FALSE(window->IsVisible());

  window->Show();

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(window->HasFocus());
}

TEST_F(LockActionHandlerLayoutManagerTest, MultipleMonitors) {
  UpdateDisplay("300x400,400x500");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();

  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);

  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW);
  widget_params.show_state = ui::SHOW_STATE_FULLSCREEN;
  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      widget_params, kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  gfx::Rect target_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      kShelfSize /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  EXPECT_EQ(root_windows[0], window->GetRootWindow());

  wm::WindowState* window_state = wm::GetWindowState(window.get());
  window_state->SetRestoreBoundsInScreen(gfx::Rect(400, 0, 30, 40));
  window_state->Maximize();

  // Maximize the window with as the restore bounds is inside 2nd display but
  // lock container windows are always on primary display.
  EXPECT_EQ(root_windows[0], window->GetRootWindow());
  target_bounds = gfx::Rect(300, 400);
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      kShelfSize /* bottom */);
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  window_state->Restore();
  EXPECT_EQ(root_windows[0], window->GetRootWindow());
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  window_state->SetRestoreBoundsInScreen(gfx::Rect(280, 0, 30, 40));
  window_state->Maximize();
  EXPECT_EQ(root_windows[0], window->GetRootWindow());
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  window_state->Restore();
  EXPECT_EQ(root_windows[0], window->GetRootWindow());
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());

  window->SetBoundsInScreen(gfx::Rect(0, 0, 30, 40), GetSecondaryDisplay());
  target_bounds = gfx::Rect(400, 500);
  target_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                      kShelfSize /* bottom */);
  target_bounds.Offset(300, 0);
  EXPECT_EQ(root_windows[1], window->GetRootWindow());
  EXPECT_EQ(target_bounds.ToString(), window->GetBoundsInScreen().ToString());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       WindowAddedWhileBackgroundShowing) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  EXPECT_EQ(LockScreenActionBackgroundState::kShowing,
            background_controller()->state());
  EXPECT_FALSE(window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  // Move action to active - this will make the app window showable. Though,
  // showing the window should be delayed until the background finishes
  // showing.
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);

  EXPECT_FALSE(window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  // Finish showing the background - this should make the app window visible.
  ASSERT_TRUE(background_controller()->FinishShow());

  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(window->HasFocus());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  // Move action state back to available - this should hide the app window, and
  // request the background window to be hidden.
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kAvailable);

  EXPECT_FALSE(window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  ASSERT_TRUE(background_controller()->FinishHide());

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(background_controller()->window()->IsVisible());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       WindowAddedWhenBackgroundShown) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);
  // Finish showing the background - this should make the app window visible
  // once it's created.
  ASSERT_TRUE(background_controller()->FinishShow());

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());
  EXPECT_EQ(LockScreenActionBackgroundState::kShown,
            background_controller()->state());

  // Move action state back to not available - this should immediately hide both
  // the app and background window,
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(background_controller()->window()->IsVisible());
  EXPECT_EQ(LockScreenActionBackgroundState::kHidden,
            background_controller()->state());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       SecondWindowAddedWhileShowingBackground) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  EXPECT_EQ(LockScreenActionBackgroundState::kShowing,
            background_controller()->state());

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  EXPECT_FALSE(window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  std::unique_ptr<aura::Window> second_window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(second_window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  // Finish showing the background. The app windows should be not be shown yet,
  // as the note actio is not active.
  ASSERT_TRUE(background_controller()->FinishShow());

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(second_window->IsVisible());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);

  EXPECT_TRUE(window->IsVisible());
  EXPECT_FALSE(window->HasFocus());
  EXPECT_TRUE(second_window->IsVisible());
  EXPECT_TRUE(second_window->HasFocus());
  EXPECT_TRUE(background_controller()->window()->IsVisible());

  // Deactivate the action - the windows should get hidden.
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kAvailable);

  EXPECT_FALSE(window->IsVisible());
  EXPECT_FALSE(second_window->IsVisible());
  EXPECT_EQ(LockScreenActionBackgroundState::kHiding,
            background_controller()->state());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       RelaunchWhilehidingBackground) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  ASSERT_TRUE(background_controller()->FinishShow());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kAvailable);
  ASSERT_EQ(LockScreenActionBackgroundState::kHiding,
            background_controller()->state());

  // Create new app window to show.
  window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);
  ASSERT_EQ(LockScreenActionBackgroundState::kShowing,
            background_controller()->state());

  EXPECT_TRUE(background_controller()->window()->IsVisible());
  EXPECT_FALSE(window->IsVisible());

  background_controller()->FinishShow();

  EXPECT_TRUE(background_controller()->window()->IsVisible());
  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(window->HasFocus());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       ActionDeactivatedWhileShowingTheBackground) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  EXPECT_TRUE(background_controller()->window()->IsVisible());
  EXPECT_FALSE(window->IsVisible());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kAvailable);

  EXPECT_TRUE(background_controller()->window()->IsVisible());
  EXPECT_FALSE(window->IsVisible());
  EXPECT_EQ(LockScreenActionBackgroundState::kHiding,
            background_controller()->state());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       ActionDisabledWhileShowingTheBackground) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kLaunching);

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  EXPECT_TRUE(background_controller()->window()->IsVisible());
  EXPECT_FALSE(window->IsVisible());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);

  EXPECT_FALSE(background_controller()->window()->IsVisible());
  EXPECT_FALSE(window->IsVisible());
}

TEST_F(LockActionHandlerLayoutManagerWithBackgroundTest,
       BackgroundWindowBounds) {
  SetUpTrayActionClientAndLockSession(mojom::TrayActionState::kActive);
  ASSERT_TRUE(background_controller()->FinishShow());

  std::unique_ptr<aura::Window> window = CreateTestingWindow(
      views::Widget::InitParams(views::Widget::InitParams::TYPE_WINDOW),
      kShellWindowId_LockActionHandlerContainer,
      std::make_unique<TestWindowDelegate>(true /*can_activate*/));

  ASSERT_TRUE(window->IsVisible());
  ASSERT_TRUE(background_controller()->window()->IsVisible());

  // Verify that the window bounds are equal to work area for the bottom shelf
  // alignment, which matches how the shelf is aligned on the lock screen,
  gfx::Rect target_app_window_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  target_app_window_bounds.Inset(0 /* left */, 0 /* top */, 0 /* right */,
                                 kShelfSize /* bottom */);
  EXPECT_EQ(target_app_window_bounds, window->GetBoundsInScreen());

  EXPECT_EQ(display::Screen::GetScreen()->GetPrimaryDisplay().bounds(),
            background_controller()->window()->GetBoundsInScreen());
}

}  // namespace ash
