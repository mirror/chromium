// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/hung_renderer_view.h"

#include <string>

#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

// Interactive UI tests for the hung renderer (aka page unresponsive) dialog.
// TODO(tapted): The framework sometimes doesn't pick up the spawned dialog and
// the ASSERT_EQ in TestBrowserUi::ShowAndVerifyUi() fails. This seems to only
// happen on the bots. So these tests are disabled for now.
class HungRendererDialogViewBrowserTest : public DialogBrowserTest {
 public:
  HungRendererDialogViewBrowserTest() {}

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    HungRendererDialogView::Show(web_contents);

    if (name == "MultiplePages") {
      auto* web_contents2 = chrome::DuplicateTabAt(browser(), 0);
      HungRendererDialogView::GetInstance()->AddWebContentsForTesting(
          web_contents2);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HungRendererDialogViewBrowserTest);
};

IN_PROC_BROWSER_TEST_F(HungRendererDialogViewBrowserTest,
                       DISABLED_InvokeUi_Default) {
  ShowAndVerifyUi();
}

IN_PROC_BROWSER_TEST_F(HungRendererDialogViewBrowserTest,
                       DISABLED_InvokeUi_MultiplePages) {
  ShowAndVerifyUi();
}
