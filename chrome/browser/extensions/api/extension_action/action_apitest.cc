// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "extensions/test/result_catcher.h"

namespace extensions {
namespace {

class ActionApiTest : public ExtensionApiTest {
 public:
  ActionApiTest() {}
  ~ActionApiTest() override {}

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    // host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  ExtensionAction* GetAction(const Extension& extension) {
    return ExtensionActionManager::Get(browser()->profile())
        ->GetAction(extension);
  }

 private:
  // std::unique_ptr<ActionTestUtil> browser_action_test_util_;

  DISALLOW_COPY_AND_ASSIGN(ActionApiTest);
};

// Tests the default disabled states.
IN_PROC_BROWSER_TEST_F(ActionApiTest, DisabledState) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("action/disabled_state")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Page action icon is displayed when a tab is created.
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/simple.html"));
  chrome::NewTab(browser());
  browser()->tab_strip_model()->ActivateTabAt(0, true);

  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action);

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);

  EXPECT_FALSE(action->GetIsVisible(ExtensionTabUtil::GetTabId(tab)));

  {
    // Tell the extension to update the page action state.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("update.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  tab = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);

  EXPECT_TRUE(action->GetIsVisible(ExtensionTabUtil::GetTabId(tab)));
  // Test that there is a browser action in the toolbar.
  // ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());

  // Check that the icon is greyed out by default.

  // Visit site. Check that it becomes enabled.

  // Navigate away. Check that it is disabled. */
}

// Tests the default enabled and disabled states.
IN_PROC_BROWSER_TEST_F(ActionApiTest, EnabledState) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("action/simple")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Page action icon is displayed when a tab is created.
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/simple.html"));
  chrome::NewTab(browser());
  browser()->tab_strip_model()->ActivateTabAt(0, true);

  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action);

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);

  EXPECT_FALSE(action->GetIsVisible(ExtensionTabUtil::GetTabId(tab)));

  // Test that there is a browser action in the toolbar.
  // ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());

  // Check that the icon is greyed out by default.

  // Visit site. Check that it becomes enabled.

  // Navigate away. Check that it is disabled.
}

}  // namespace
}  // namespace extensions
