// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/sad_tab_view.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/result_codes.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/widget/widget.h"

class SadTabViewInteractiveUITest : public InProcessBrowserTest {
 public:
  SadTabViewInteractiveUITest() {}

 protected:
  void PressTab() {
    ASSERT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_TAB, false,
                                                false, false, false));
  }

  void PressShiftTab() {
    ASSERT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_TAB, false,
                                                true, false, false));
  }

  views::FocusManager* GetFocusManager() {
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->GetWidget()->GetFocusManager();
  }

  views::View* GetFocusedView() { return GetFocusManager()->GetFocusedView(); }

  bool IsFocusedViewInsideViewClass(const char* view_class) {
    views::View* view = GetFocusedView();
    while (view) {
      if (view->GetClassName() == view_class)
        return true;
      view = view->parent();
    }
    return false;
  }

  bool IsFocusedViewInsideSadTab() {
    return IsFocusedViewInsideViewClass(SadTabView::kViewClassName);
  }

  bool IsFocusedViewInsideBrowserToolbar() {
    return IsFocusedViewInsideViewClass(ToolbarView::kViewClassName);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SadTabViewInteractiveUITest);
};

IN_PROC_BROWSER_TEST_F(SadTabViewInteractiveUITest,
                       SadTabKeyboardAccessibility) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL("/links.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  // Start with focus in the location bar.
  chrome::FocusLocationBar(browser());
  ASSERT_FALSE(IsFocusedViewInsideSadTab());
  ASSERT_TRUE(IsFocusedViewInsideBrowserToolbar());

  // Kill the renderer process, resulting in a sad tab.
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderProcessHost* process =
      web_contents->GetMainFrame()->GetProcess();
  content::RenderProcessHostWatcher crash_observer(
      process, content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
  process->Shutdown(content::RESULT_CODE_KILLED, false);
  crash_observer.Wait();

  // Focus should now be on a label button inside the sad tab.
  ASSERT_STREQ(GetFocusedView()->GetClassName(),
               views::LabelButton::kViewClassName);
  ASSERT_TRUE(IsFocusedViewInsideSadTab());
  ASSERT_FALSE(IsFocusedViewInsideBrowserToolbar());

  // Pressing the Tab key should cycle focus back to the toolbar.
  PressTab();
  ASSERT_FALSE(IsFocusedViewInsideSadTab());
  ASSERT_TRUE(IsFocusedViewInsideBrowserToolbar());

  // Keep pressing the Tab key and make sure we make it back to the sad tab.
  while (!IsFocusedViewInsideSadTab())
    PressTab();
  ASSERT_FALSE(IsFocusedViewInsideBrowserToolbar());

  // Press Shift-Tab and ensure we end up back in the toolbar.
  PressShiftTab();
  ASSERT_FALSE(IsFocusedViewInsideSadTab());
  ASSERT_TRUE(IsFocusedViewInsideBrowserToolbar());
}
