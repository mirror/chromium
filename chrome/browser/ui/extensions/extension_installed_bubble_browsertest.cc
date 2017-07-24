// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/auto_reset.h"
#include "chrome/browser/extensions/extension_action_test_util.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/extensions/extension_installed_bubble.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "components/signin/core/browser/signin_manager.h"

using extensions::extension_action_test_util::ActionType;

class ExtensionInstalledBubbleBrowserTest
    : public SupportsTestDialog<ExtensionBrowserTest> {
 public:
  ExtensionInstalledBubbleBrowserTest()
      : disable_animations_(&ToolbarActionsBar::disable_animations_for_testing_,
                            true) {}

  std::unique_ptr<ExtensionInstalledBubble> MakeBubble(const std::string& name,
                                                       ActionType type);

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override;

 private:
  base::AutoReset<bool> disable_animations_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstalledBubbleBrowserTest);
};

std::unique_ptr<ExtensionInstalledBubble>
ExtensionInstalledBubbleBrowserTest::MakeBubble(const std::string& name,
                                                ActionType type) {
  const SkBitmap kEmptyBitmap;
  scoped_refptr<const extensions::Extension> extension =
      extensions::extension_action_test_util::CreateActionExtension(name, type);
  extension_service()->AddExtension(extension.get());
  auto bubble = base::MakeUnique<ExtensionInstalledBubble>(
      extension.get(), browser(), SkBitmap());
  bubble->Initialize();
  return bubble;
}

void ExtensionInstalledBubbleBrowserTest::ShowDialog(const std::string& name) {
  ActionType type = ActionType::NO_ACTION;
  if (name == "BrowserAction")
    type = ActionType::BROWSER_ACTION;
  else if (name == "PageAction")
    type = ActionType::PAGE_ACTION;
  else if (name == "InstalledByDefault")
    type = ActionType::INSTALLED_BY_DEFAULT;
  else if (name == "Omnibox")
    type = ActionType::OMNIBOX;
  auto bubble = MakeBubble(name, type);
  BubbleManager* manager = browser()->GetBubbleManager();
  manager->ShowBubble(std::move(bubble));
}

IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       InvokeDialog_NoAction) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       InvokeDialog_BrowserAction) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       InvokeDialog_PageAction) {
  RunDialog();
}

// Test anchoring to the app menu.
IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       InvokeDialog_InstalledByDefault) {
  RunDialog();
}

// Test anchoring to the omnibox.
IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       InvokeDialog_Omnibox) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       InvokeDialog_SignedIn) {
  SigninManagerFactory::GetForProfile(browser()->profile())
      ->SetAuthenticatedAccountInfo("test", "test@example.com");
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(ExtensionInstalledBubbleBrowserTest,
                       DoNotShowHowToUseForSynthesizedActions) {
  {
    auto bubble = MakeBubble("No action", ActionType::NO_ACTION);
    EXPECT_EQ(0, bubble->options() & ExtensionInstalledBubble::HOW_TO_USE);
  }
  {
    auto bubble = MakeBubble("Browser action", ActionType::BROWSER_ACTION);
    EXPECT_NE(0, bubble->options() & ExtensionInstalledBubble::HOW_TO_USE);
  }
  {
    auto bubble = MakeBubble("Page action", ActionType::PAGE_ACTION);
    EXPECT_NE(0, bubble->options() & ExtensionInstalledBubble::HOW_TO_USE);
  }
}
