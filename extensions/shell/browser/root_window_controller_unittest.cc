// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/root_window_controller.h"

#include <memory>

#include "base/macros.h"
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
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/base/ime/input_method_minimal.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace extensions {

namespace {

class FakeDesktopDelegate : public RootWindowController::DesktopDelegate {
 public:
  FakeDesktopDelegate() {
    root_window_controller_ = base::MakeUnique<RootWindowController>(this);
  }

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
  RootWindowControllerTest() {}
  ~RootWindowControllerTest() override {}

  void SetUp() override {
    ShellTestBaseAura::SetUp();
    screen_ = base::MakeUnique<ShellScreen>(nullptr, screen_bounds_.size());
    display::Screen::SetScreenInstance(screen_.get());
  }

  void TearDown() override {
    display::Screen::SetScreenInstance(nullptr);
    screen_.reset();
    ShellTestBaseAura::TearDown();
  }

 protected:
  ShellScreen* screen() { return screen_.get(); }

 private:
  std::unique_ptr<ShellScreen> screen_;
  const gfx::Rect screen_bounds_ = gfx::Rect(0, 0, 800, 600);

  DISALLOW_COPY_AND_ASSIGN(RootWindowControllerTest);
};

// Tests RWC.
TEST_F(RootWindowControllerTest, Basic) {
  FakeDesktopDelegate desktop_delegate;
  RootWindowController* root_window_controller =
      desktop_delegate.root_window_controller();

  EXPECT_FALSE(root_window_controller->host());
  root_window_controller->CreateWindowTreeHost(screen(),
                                               gfx::Rect(0, 0, 800, 600));
  EXPECT_TRUE(root_window_controller->host());

  // The RootWindowController destroys itself when the root window closes.
  root_window_controller->OnHostCloseRequested(root_window_controller->host());
  EXPECT_FALSE(desktop_delegate.root_window_controller());
}

TEST_F(RootWindowControllerTest, AppWindows) {
  FakeDesktopDelegate desktop_delegate;
  RootWindowController* root_window_controller =
      desktop_delegate.root_window_controller();

  root_window_controller->CreateWindowTreeHost(screen(),
                                               gfx::Rect(0, 0, 800, 600));
  EXPECT_TRUE(root_window_controller->host());

  TestAppWindowClient app_window_client;
  AppWindowClient::Set(&app_window_client);

  scoped_refptr<Extension> extension = test_util::CreateEmptyExtension();

  // Create some AppWindows.
  std::vector<AppWindow*> app_windows;
  for (int i = 0; i < 3; i++) {
    AppWindow* app_window = root_window_controller->CreateAppWindow(
        browser_context(), extension.get());
    InitAppWindow(app_window);
    root_window_controller->AddAppWindow(app_window->GetNativeWindow());
    app_windows.push_back(app_window);
  }
  EXPECT_EQ(3u,
            AppWindowRegistry::Get(browser_context())->app_windows().size());
  EXPECT_EQ(3u, root_window_controller->host()->window()->children().size());

  // Close one window.
  app_windows[1]->OnNativeClose();
  EXPECT_EQ(2u, root_window_controller->host()->window()->children().size());

  root_window_controller->CloseAppWindows();
  EXPECT_TRUE(AppWindowRegistry::Get(browser_context())->app_windows().empty());
  EXPECT_TRUE(root_window_controller->host()->window()->children().empty());

  AppWindowClient::Set(nullptr);
}

}  // namespace extensions
