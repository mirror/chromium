// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/root_window_controller.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "extensions/common/test_util.h"
#include "extensions/shell/browser/root_window_controller.h"
#include "extensions/shell/browser/shell_app_window_client.h"
#include "extensions/shell/browser/shell_native_app_window_aura.h"
#include "extensions/shell/browser/shell_screen.h"
#include "extensions/shell/test/shell_test_base_aura.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace extensions {

namespace {

// A fake that creates and exposes a RootWindowController.
class FakeDesktopDelegate : public RootWindowController::DesktopDelegate {
 public:
  FakeDesktopDelegate(ShellScreen* screen, const gfx::Rect& bounds) {
    root_window_controller_ =
        base::MakeUnique<RootWindowController>(this, screen, bounds);
  }
  ~FakeDesktopDelegate() override = default;

  // RootWindowController::DesktopDelegate:
  void CloseRootWindowController(
      RootWindowController* root_window_controller) override {
    EXPECT_EQ(root_window_controller_.get(), root_window_controller);
    root_window_controller_.reset();
  }

  RootWindowController* root_window_controller() {
    return root_window_controller_.get();
  }

 private:
  std::unique_ptr<RootWindowController> root_window_controller_;

  DISALLOW_COPY_AND_ASSIGN(FakeDesktopDelegate);
};

// An AppWindowClient for use without a DesktopController.
class TestAppWindowClient : public ShellAppWindowClient {
 public:
  TestAppWindowClient() = default;
  ~TestAppWindowClient() override = default;

  NativeAppWindow* CreateNativeAppWindow(
      AppWindow* window,
      AppWindow::CreateParams* params) override {
    return new ShellNativeAppWindowAura(window, *params);
  }
};

}  // namespace

class RootWindowControllerTest : public ShellTestBaseAura {
 public:
  RootWindowControllerTest() = default;
  ~RootWindowControllerTest() override = default;

  void SetUp() override {
    ShellTestBaseAura::SetUp();

    AppWindowClient::Set(&app_window_client_);
    screen_ = base::MakeUnique<ShellScreen>(nullptr, screen_bounds_.size());
    display::Screen::SetScreenInstance(screen_.get());
    extension_ = test_util::CreateEmptyExtension();

    desktop_delegate_ =
        base::MakeUnique<FakeDesktopDelegate>(screen_.get(), screen_bounds_);
    EXPECT_TRUE(root_window_controller()->host());
  }

  void TearDown() override {
    desktop_delegate_.reset();
    display::Screen::SetScreenInstance(nullptr);
    screen_.reset();
    AppWindowClient::Set(nullptr);
    ShellTestBaseAura::TearDown();
  }

  // Creates and returns an AppWindow using the RootWindowController.
  AppWindow* CreateAppWindow() {
    AppWindow* app_window = root_window_controller()->CreateAppWindow(
        browser_context(), extension());
    InitAppWindow(app_window);
    root_window_controller()->AddAppWindow(app_window->GetNativeWindow());
    return app_window;
  }

  // Checks that there are |num_expected| AppWindows open.
  void ExpectNumAppWindows(size_t expected) {
    EXPECT_EQ(expected,
              root_window_controller()->host()->window()->children().size());
    EXPECT_EQ(expected,
              AppWindowRegistry::Get(browser_context())->app_windows().size());
  }

 protected:
  RootWindowController* root_window_controller() {
    return desktop_delegate_->root_window_controller();
  }

  const Extension* extension() { return extension_.get(); }

 private:
  TestAppWindowClient app_window_client_;
  const gfx::Rect screen_bounds_ = gfx::Rect(0, 0, 800, 600);

  scoped_refptr<Extension> extension_;
  std::unique_ptr<ShellScreen> screen_;
  std::unique_ptr<FakeDesktopDelegate> desktop_delegate_;

  DISALLOW_COPY_AND_ASSIGN(RootWindowControllerTest);
};

// Tests RootWindowController's basic setup and teardown.
TEST_F(RootWindowControllerTest, Basic) {
  // The RootWindowController destroys itself when the root window closes.
  root_window_controller()->OnHostCloseRequested(
      root_window_controller()->host());
  EXPECT_FALSE(root_window_controller());
}

// Tests the window layout.
TEST_F(RootWindowControllerTest, FillLayout) {
  root_window_controller()->host()->SetBoundsInPixels(
      gfx::Rect(0, 0, 500, 700));

  AppWindow* app_window =
      root_window_controller()->CreateAppWindow(browser_context(), extension());
  InitAppWindow(app_window);
  root_window_controller()->AddAppWindow(app_window->GetNativeWindow());

  ExpectNumAppWindows(1u);

  // Test that reshaping the host window also resizes the child window.
  root_window_controller()->host()->SetBoundsInPixels(
      gfx::Rect(0, 0, 400, 300));

  const aura::Window* root_window = root_window_controller()->host()->window();
  EXPECT_EQ(400, root_window->bounds().width());
  EXPECT_EQ(400, root_window->children()[0]->bounds().width());

  // The AppWindow will close on shutdown.
}

// Tests creating and removing AppWindows.
TEST_F(RootWindowControllerTest, AppWindows) {
  // Create some AppWindows.
  CreateAppWindow();
  AppWindow* middle_window = CreateAppWindow();
  CreateAppWindow();

  ExpectNumAppWindows(3u);

  // Close one window.
  middle_window->GetBaseWindow()->Close();
  middle_window = nullptr;  // Close() deleted the AppWindow.
  ExpectNumAppWindows(2u);

  // Close all remaining windows.
  root_window_controller()->CloseAppWindows();
  ExpectNumAppWindows(0u);
}

}  // namespace extensions
