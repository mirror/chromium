// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"

#include "base/command_line.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#import "ui/base/test/scoped_fake_nswindow_fullscreen.h"
#include "ui/base/ui_base_switches.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/widget/widget.h"
#include "url/url_constants.h"

namespace {

class PageInfoBubbleViewsMacTest : public InProcessBrowserTest {
 public:
  PageInfoBubbleViewsMacTest() {}

  // content::BrowserTestBase:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kExtendMdToSecondaryUi);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PageInfoBubbleViewsMacTest);
};

}  // namespace

// Test the Page Info bubble doesn't crash upon entering full screen mode while
// it is open. This may occur when the bubble is trying to reanchor itself.
IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewsMacTest, NoCrashOnFullScreenToggle) {
  ui::test::ScopedFakeNSWindowFullscreen fake_fullscreen;
  ui_test_utils::NavigateToURL(browser(), GURL(url::kAboutBlankURL));
  ShowPageInfoDialog(browser()->tab_strip_model()->GetWebContentsAt(0));
  ExclusiveAccessManager* access_manager =
      browser()->exclusive_access_manager();
  FullscreenController* fullscreen_controller =
      access_manager->fullscreen_controller();

  views::BubbleDialogDelegateView* page_info =
      PageInfoBubbleView::GetPageInfoBubble();
  EXPECT_TRUE(page_info);
  page_info->set_close_on_deactivate(false);
  views::Widget* page_info_bubble = page_info->GetWidget();
  EXPECT_TRUE(page_info_bubble);
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_PAGE_INFO,
            PageInfoBubbleView::GetShownBubbleType());

  fullscreen_controller->ToggleBrowserFullscreenMode();
  EXPECT_TRUE(page_info_bubble->IsVisible());
  // There should be no crash here from re-anchoring the Page Info bubble while
  // transitioning into full screen.
  fake_fullscreen.FinishTransition();
}
