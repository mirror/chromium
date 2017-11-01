// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/hung_renderer_view.h"

#include <string>

#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents_unresponsive_state.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/controllable_http_response.h"

namespace {

const char kSimplePage[] =
    "data:text/html;charset=utf-8,"
    "<!DOCTYPE html>"
    "<meta name='viewport' content='width=device-width'/>"
    "<html><body>response's body</body></html>";

}  // namespace

class HungRendererDialogViewBrowserTest : public DialogBrowserTest {
 public:
  HungRendererDialogViewBrowserTest() {}

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override {
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    TabDialogs::FromWebContents(web_contents)
        ->ShowHungRendererDialog(content::WebContentsUnresponsiveState());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HungRendererDialogViewBrowserTest);
};

class HungRendererViewNavigationBrowserTest : public InProcessBrowserTest {
 public:
  HungRendererViewNavigationBrowserTest() {}
  ~HungRendererViewNavigationBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(HungRendererViewNavigationBrowserTest);
};

// Invokes the hung renderer (aka page unresponsive) dialog. See
// test_browser_dialog.h.
// TODO(tapted): The framework sometimes doesn't pick up the spawned dialog and
// the ASSERT_EQ in TestBrowserDialog::RunDialog() fails. This seems to only
// happen on the bots. So the test is disabled for now.
IN_PROC_BROWSER_TEST_F(HungRendererDialogViewBrowserTest,
                       DISABLED_InvokeDialog_default) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(HungRendererViewNavigationBrowserTest,
                       HungRendererWithNavigation) {
  EXPECT_TRUE(embedded_test_server()->Start());

  const GURL initial_url(kSimplePage);
  ui_test_utils::NavigateToURL(browser(), initial_url);
  content::WebContents* active_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  TabDialogs::FromWebContents(active_web_contents)
      ->ShowHungRendererDialog(content::WebContentsUnresponsiveState());
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("example.com", "/sample.html"));
  // Expect that the dialog has been dismissed.
  EXPECT_FALSE(HungRendererDialogView::GetInstance());
}
