// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands_mac.h"
#import "chrome/browser/ui/cocoa/permission_bubble/permission_bubble_cocoa.h"
#import "chrome/browser/ui/cocoa/permission_bubble/permission_bubble_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/permission_bubble/permission_bubble_browser_test_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/filename_util.h"
#import "testing/gtest_mac.h"
#import "ui/base/cocoa/fullscreen_window_manager.h"
#include "ui/base/test/scoped_fake_nswindow_fullscreen.h"
#include "url/gurl.h"

namespace {
class MockPermissionBubbleCocoa : public PermissionBubbleCocoa {
 public:
  MockPermissionBubbleCocoa(NSWindow* parent_window)
      : PermissionBubbleCocoa(parent_window) {}

  MOCK_METHOD0(HasLocationBar, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPermissionBubbleCocoa);
};

class ImmersiveFullscreenMacBrowserTest : public PermissionBubbleBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ui_test_utils::NavigateToURL(browser(), GetFullscreenTestPageURL());
    PermissionBubbleBrowserTest::SetUpOnMainThread();
  }

 protected:
  void EnterFullscreen() { Transition(base::ASCIIToUTF16("fullscreen")); }

  void ExitFullscreen() { Transition(base::ASCIIToUTF16("not-fullscreen")); }

 private:
  void Transition(const base::string16& expected_title) {
    auto web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    content::TitleWatcher watcher(web_contents, expected_title);
    MouseClick(web_contents);
    ASSERT_EQ(watcher.WaitAndGetTitle(), expected_title);
  }

  // "fullscreen_test.html" will toggle fullscreen when clicked.
  void MouseClick(content::WebContents* web_contents) {
    content::SimulateMouseClick(
        browser()->tab_strip_model()->GetActiveWebContents(), 0,
        blink::WebMouseEvent::ButtonLeft);
  }

  GURL GetFullscreenTestPageURL() {
    base::FilePath path;

    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("chrome");
    path = path.Append(FILE_PATH_LITERAL("test"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    path = path.Append(FILE_PATH_LITERAL("fullscreen_test.html"));

    return net::FilePathToFileURL(path);
  }
};
}

IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest, HasLocationBarByDefault) {
  PermissionBubbleCocoa bubble(browser());
  bubble.SetDelegate(test_delegate());
  bubble.Show();
  EXPECT_TRUE([bubble.bubbleController_ hasVisibleLocationBar]);
  bubble.Hide();
}

IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest,
                       BrowserFullscreenHasLocationBar) {
  ui::test::ScopedFakeNSWindowFullscreen faker;

  PermissionBubbleCocoa bubble(browser());
  bubble.SetDelegate(test_delegate());
  bubble.Show();
  EXPECT_TRUE([bubble.bubbleController_ hasVisibleLocationBar]);

  FullscreenController* controller =
      browser()->exclusive_access_manager()->fullscreen_controller();
  controller->ToggleBrowserFullscreenMode();
  faker.FinishTransition();

  // The location bar should be visible if the toolbar is set to be visible in
  // fullscreen mode.
  PrefService* prefs = browser()->profile()->GetPrefs();
  bool show_toolbar = prefs->GetBoolean(prefs::kShowFullscreenToolbar);
  EXPECT_EQ(show_toolbar, [bubble.bubbleController_ hasVisibleLocationBar]);

  // Toggle the value of the preference.
  chrome::ToggleFullscreenToolbar(browser());
  EXPECT_EQ(!show_toolbar, [bubble.bubbleController_ hasVisibleLocationBar]);

  // Put the setting back the way it started.
  chrome::ToggleFullscreenToolbar(browser());
  controller->ToggleBrowserFullscreenMode();
  faker.FinishTransition();

  EXPECT_TRUE([bubble.bubbleController_ hasVisibleLocationBar]);
  bubble.Hide();
}

IN_PROC_BROWSER_TEST_F(ImmersiveFullscreenMacBrowserTest, Bug) {
  EnterFullscreen();
  ExitFullscreen();
}

IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest,
                       TabFullscreenHasLocationBar) {
  ui::test::ScopedFakeNSWindowFullscreen faker;

  PermissionBubbleCocoa bubble(browser());
  bubble.SetDelegate(test_delegate());
  bubble.Show();
  EXPECT_TRUE([bubble.bubbleController_ hasVisibleLocationBar]);

  FullscreenController* controller =
      browser()->exclusive_access_manager()->fullscreen_controller();
  controller->EnterFullscreenModeForTab(
      browser()->tab_strip_model()->GetActiveWebContents(), GURL());
  faker.FinishTransition();

  EXPECT_FALSE([bubble.bubbleController_ hasVisibleLocationBar]);
  controller->ExitFullscreenModeForTab(
      browser()->tab_strip_model()->GetActiveWebContents());
  faker.FinishTransition();

  EXPECT_TRUE([bubble.bubbleController_ hasVisibleLocationBar]);
  bubble.Hide();
}

IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest, AppHasNoLocationBar) {
  Browser* app_browser = OpenExtensionAppWindow();
  PermissionBubbleCocoa bubble(app_browser);
  bubble.SetDelegate(test_delegate());
  bubble.Show();
  EXPECT_FALSE([bubble.bubbleController_ hasVisibleLocationBar]);
  bubble.Hide();
}

// http://crbug.com/470724
// Kiosk mode on Mac has a location bar but it shouldn't.
IN_PROC_BROWSER_TEST_F(PermissionBubbleKioskBrowserTest,
                       DISABLED_KioskHasNoLocationBar) {
  PermissionBubbleCocoa bubble(browser());
  bubble.SetDelegate(test_delegate());
  bubble.Show();
  EXPECT_FALSE([bubble.bubbleController_ hasVisibleLocationBar]);
  bubble.Hide();
}
