// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/ui_test_utils.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/events/keycodes/keyboard_codes.h"

using UiTestUtilsTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(UiTestUtilsTest,
    DISABLED_NavigateToURLWithDispositionShouldSynchronouslyLoadDataHtml) {
  static const std::string page =
      "<html><head></head><body></body><script>"
      "document.addEventListener('keydown', (e) => {"
      "document.body.webkitRequestFullscreen(); });"
      "</script></html>";
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL("data:text/html," + page),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      browser(), ui::VKEY_0, false, false, false, false));
  // If the NavigateToURLWithDisposition() can wait until the page is loaded,
  // document.webkitFullscreenElement.tagName should be "body".
  std::string result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      browser()->tab_strip_model()->GetActiveWebContents()->GetRenderViewHost(),
      "window.domAutomationController.send("
          "document.webkitFullscreenElement.tagName.toLowerCase());",
      &result));
  ASSERT_EQ("body", result);
}
