// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_keyboard_ui.h"

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_content_client_initializer.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"

class TestChromeKeyboardUI : public ChromeKeyboardUI {
 public:
  TestChromeKeyboardUI(content::BrowserContext* context,
                       content::WebContents* contents)
      : ChromeKeyboardUI(context), contents_(contents) {}
  ~TestChromeKeyboardUI() override {}

  ui::InputMethod* GetInputMethod() override { return nullptr; }
  void RequestAudioInput(content::WebContents* web_contents,
                         const content::MediaStreamRequest& request,
                         const content::MediaResponseCallback& callback) {}

  content::WebContents* CreateWebContents() override { return contents_; }

 private:
  content::WebContents* contents_;
};

class ChromeKeyboardUITest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    initializer_ = base::MakeUnique<content::TestContentClientInitializer>();
    content::RenderViewHostTestHarness::SetUp();
  }

  void TearDown() override {
    content::RenderViewHostTestHarness::TearDown();
    initializer_.reset();
  }

 private:
  std::unique_ptr<content::TestContentClientInitializer> initializer_;
};

// A test for crbug.com/734534
TEST_F(ChromeKeyboardUITest, DoesNotCrashWhenParentDoesNotExist) {
  content::WebContents* contents = CreateTestWebContents();
  TestChromeKeyboardUI keyboard_ui(contents->GetBrowserContext(), contents);

  EXPECT_FALSE(keyboard_ui.HasContentsWindow());
  aura::Window* view = keyboard_ui.GetContentsWindow();
  EXPECT_TRUE(keyboard_ui.HasContentsWindow());

  EXPECT_FALSE(view->parent());

  // Change window size to trigger OnWindowBoundsChanged.
  view->SetBounds(gfx::Rect(0, 0, 1200, 800));
}
