// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/ui_test_utils.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "ui/events/keycodes/keyboard_codes.h"

class UiTestUtilsTest : public InProcessBrowserTest {
 public:
  UiTestUtilsTest() = default;
  ~UiTestUtilsTest() override = default;

 protected:
  Browser* GetActiveBrowser() const;
  content::WebContents* GetActiveWebContents() const;

  void TestInCurrentTab();

  void SendShortcut(ui::KeyboardCode key);

 private:
  void SetUpOnMainThread() override;
};

Browser* UiTestUtilsTest::GetActiveBrowser() const {
  return BrowserList::GetInstance()->GetLastActive();
}

content::WebContents* UiTestUtilsTest::GetActiveWebContents() const {
  return GetActiveBrowser()->tab_strip_model()->GetActiveWebContents();
}

void UiTestUtilsTest::TestInCurrentTab() {
  static const std::string page =
      "<html><head></head><body></body><script>"
      "document.addEventListener('keydown', (e) => {"
      "document.body.webkitRequestFullscreen(); });"
      "</script></html>";
  ui_test_utils::NavigateToURLWithDisposition(
      GetActiveBrowser(),
      GURL("data:text/html," + page),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(), ui::VKEY_0, false, false, false, false));
  observer.Wait();
  std::string result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      GetActiveWebContents()->GetRenderViewHost(),
      "window.domAutomationController.send("
          "document.webkitFullscreenElement.tagName.toLowerCase());",
      &result));
  ASSERT_EQ("body", result);
}

void UiTestUtilsTest::SendShortcut(ui::KeyboardCode key) {
#if defined(OS_MACOSX)
  const bool control_modifier = false;
  const bool command_modifier = true;
#else
  const bool control_modifier = true;
  const bool command_modifier = false;
#endif
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      GetActiveBrowser(),
      key,
      control_modifier,
      false,
      false,
      command_modifier));
}

void UiTestUtilsTest::SetUpOnMainThread() {
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(GetActiveBrowser()));
}

IN_PROC_BROWSER_TEST_F(UiTestUtilsTest,
    DISABLED_NavigateToURLShouldSynchronouslyLoadDataHtml) {
  TestInCurrentTab();
}

IN_PROC_BROWSER_TEST_F(UiTestUtilsTest,
    DISABLED_NavigateToURLShouldSynchronouslyLoadDataHtmlInNewTab) {
  const content::WebContents* const current_tab = GetActiveWebContents();
  ASSERT_NE(nullptr, current_tab);
  SendShortcut(ui::VKEY_T);
  while (current_tab == GetActiveWebContents() ||
         GetActiveBrowser()->tab_strip_model()->count() != 2) {
    content::RunAllPendingInMessageLoop();
  }
  TestInCurrentTab();
}

IN_PROC_BROWSER_TEST_F(UiTestUtilsTest,
    DISABLED_NavigateToURLShouldSynchronouslyLoadDataHtmlInNewWindow) {
  const Browser* const current_browser = GetActiveBrowser();
  SendShortcut(ui::VKEY_N);
  while (current_browser == GetActiveBrowser() ||
         BrowserList::GetInstance()->size() != 2) {
    content::RunAllPendingInMessageLoop();
  }
  TestInCurrentTab();
}
