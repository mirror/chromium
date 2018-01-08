// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/browser_action_test_util.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_icon_factory.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_action_runner.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace extensions {
namespace {

void ExecuteExtensionAction(Browser* browser, const Extension* extension) {
  ExtensionActionRunner::GetForWebContents(
      browser->tab_strip_model()->GetActiveWebContents())
      ->RunAction(extension, true);
}

class ActionApiTest : public ExtensionApiTest {
 public:
  ActionApiTest() {}
  ~ActionApiTest() override {}

 protected:
  ExtensionAction* GetAction(const Extension& extension) {
    return ExtensionActionManager::Get(browser()->profile())
        ->GetAction(extension);
  }

  BrowserActionTestUtil* GetBrowserActionsBar() {
    if (!browser_action_test_util_)
      browser_action_test_util_.reset(new BrowserActionTestUtil(browser()));
    return browser_action_test_util_.get();
  }

 private:
  std::unique_ptr<BrowserActionTestUtil> browser_action_test_util_;

  DISALLOW_COPY_AND_ASSIGN(ActionApiTest);
};

class ActionApiTestWithParam : public ActionApiTest,
                               public ::testing::WithParamInterface<bool> {};

const char kEmptyImageDataError[] =
    "The imageData property must contain an ImageData object or dictionary "
    "of ImageData objects.";
const char kEmptyPathError[] = "The path property must not be empty.";

// Makes sure |bar_rendering| has |model_icon| in the middle (there's additional
// padding that correlates to the rest of the button, and this is ignored).
void VerifyIconsMatch(const gfx::Image& bar_rendering,
                      const gfx::Image& model_icon) {
  gfx::Rect icon_portion(gfx::Point(), bar_rendering.Size());
  icon_portion.ClampToCenteredSize(model_icon.Size());

  EXPECT_TRUE(gfx::test::AreBitmapsEqual(
      model_icon.AsImageSkia().GetRepresentation(1.0f).sk_bitmap(),
      gfx::ImageSkiaOperations::ExtractSubset(bar_rendering.AsImageSkia(),
                                              icon_portion)
          .GetRepresentation(1.0f)
          .sk_bitmap()));
}

IN_PROC_BROWSER_TEST_P(ActionApiTestWithParam, Simple) {
  const bool kDefaultEnabled = GetParam();
  ASSERT_TRUE(embedded_test_server()->Start());
  std::string default_state =
      kDefaultEnabled ? "default_enabled" : "default_disabled";
  ASSERT_TRUE(RunExtensionTest("action/" + default_state + "/simple"))
      << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Action icon is displayed when a tab is created.
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/simple.html"));
  chrome::NewTab(browser());
  browser()->tab_strip_model()->ActivateTabAt(0, true);

  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action);

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);

  // The initial icon visibility should by consistent with the default state.
  EXPECT_EQ(kDefaultEnabled,
            action->GetIsVisible(ExtensionTabUtil::GetTabId(tab)));

  {
    // Tell the extension to update the action state.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("update.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  tab = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(tab);

  // The icon visibility should be the opposite of its initial state.
  EXPECT_EQ(!kDefaultEnabled,
            action->GetIsVisible(ExtensionTabUtil::GetTabId(tab)));
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, EnabledBasic) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("action/default_enabled/basics")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Tell the extension to update the action state.
  ResultCatcher catcher;
  ui_test_utils::NavigateToURL(browser(),
                               GURL(extension->GetResourceURL("update.html")));
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  // Test that we received the changes.
  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action) << "Action test extension should have an action.";
  ASSERT_EQ("Modified", action->GetTitle(ExtensionAction::kDefaultTabId));
  ASSERT_EQ("badge", action->GetBadgeText(ExtensionAction::kDefaultTabId));
  ASSERT_EQ(SkColorSetARGB(255, 255, 255, 255),
            action->GetBadgeBackgroundColor(ExtensionAction::kDefaultTabId));

  // Simulate the action being clicked.
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/extensions/test_file.txt"));

  ExecuteExtensionAction(browser(), extension);

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, DisabledBasic) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("action/default_disabled/basics")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;
  {
    // Tell the extension to update the action state.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("update.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  // Test that we received the changes.
  int tab_id = ExtensionTabUtil::GetTabId(
      browser()->tab_strip_model()->GetActiveWebContents());
  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action);
  EXPECT_EQ("Modified", action->GetTitle(tab_id));

  {
    // Simulate the action being clicked.
    ResultCatcher catcher;
    ExtensionActionRunner::GetForWebContents(
        browser()->tab_strip_model()->GetActiveWebContents())
        ->RunAction(extension, true);
    EXPECT_TRUE(catcher.GetNextResult());
  }

  {
    // Tell the extension to update the action state again.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("update2.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  // We should not be creating icons asynchronously, so we don't need an
  // observer.
  ExtensionActionIconFactory icon_factory(profile(), extension, action, NULL);

  // Test that we received the changes.
  tab_id = ExtensionTabUtil::GetTabId(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_FALSE(icon_factory.GetIcon(tab_id).IsEmpty());
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, DynamicBrowserAction) {
  ASSERT_TRUE(RunExtensionTest("action/default_enabled/no_icon")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

#if defined(OS_MACOSX)
  // We need this on mac so we don't loose 2x representations from browser icon
  // in transformations gfx::ImageSkia -> NSImage -> gfx::ImageSkia.
  std::vector<ui::ScaleFactor> supported_scale_factors;
  supported_scale_factors.push_back(ui::SCALE_FACTOR_100P);
  supported_scale_factors.push_back(ui::SCALE_FACTOR_200P);
  ui::SetSupportedScaleFactors(supported_scale_factors);
#endif

  // We should not be creating icons asynchronously, so we don't need an
  // observer.
  ExtensionActionIconFactory icon_factory(profile(), extension,
                                          GetAction(*extension), NULL);
  // Test that there is an action in the toolbar.

  gfx::Image action_icon = icon_factory.GetIcon(0);
  uint32_t action_icon_last_id = action_icon.ToSkBitmap()->getGenerationID();

  // Let's check that |GetIcon| doesn't always return bitmap with new id.
  ASSERT_EQ(action_icon_last_id,
            icon_factory.GetIcon(0).ToSkBitmap()->getGenerationID());

  gfx::Image last_bar_icon = GetBrowserActionsBar()->GetIcon(0);
  EXPECT_TRUE(gfx::test::AreImagesEqual(last_bar_icon,
                                        GetBrowserActionsBar()->GetIcon(0)));

  // The reason we don't test more standard scales (like 1x, 2x, etc.) is that
  // these may be generated from the provided scales.
  float kSmallIconScale = 21.f / ExtensionAction::ActionIconSize();
  float kLargeIconScale = 42.f / ExtensionAction::ActionIconSize();
  ASSERT_FALSE(ui::IsSupportedScale(kSmallIconScale));
  ASSERT_FALSE(ui::IsSupportedScale(kLargeIconScale));

  // Tell the extension to update the icon using ImageData object.
  ResultCatcher catcher;
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  uint32_t action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;
  VerifyIconsMatch(last_bar_icon, action_icon);

  // Check that only the smaller size was set (only a 21px icon was provided).
  EXPECT_TRUE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_FALSE(action_icon.ToImageSkia()->HasRepresentation(kLargeIconScale));

  // Tell the extension to update the icon using path.
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  // Make sure the action bar updated.
  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;
  VerifyIconsMatch(last_bar_icon, action_icon);

  // Check that only the smaller size was set (only a 21px icon was provided).
  EXPECT_TRUE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_FALSE(action_icon.ToImageSkia()->HasRepresentation(kLargeIconScale));

  // Tell the extension to update the icon using dictionary of ImageData
  // objects.
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;
  VerifyIconsMatch(last_bar_icon, action_icon);

  // Check both sizes were set (as two icon sizes were provided).
  EXPECT_TRUE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_TRUE(action_icon.AsImageSkia().HasRepresentation(kLargeIconScale));

  // Tell the extension to update the icon using dictionary of paths.
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;
  VerifyIconsMatch(last_bar_icon, action_icon);

  // Check both sizes were set (as two icon sizes were provided).
  EXPECT_TRUE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_TRUE(action_icon.AsImageSkia().HasRepresentation(kLargeIconScale));

  // Tell the extension to update the icon using dictionary of ImageData
  // objects, but setting only one size.
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;
  VerifyIconsMatch(last_bar_icon, action_icon);

  // Check that only the smaller size was set (only a 21px icon was provided).
  EXPECT_TRUE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_FALSE(action_icon.ToImageSkia()->HasRepresentation(kLargeIconScale));

  // Tell the extension to update the icon using dictionary of paths, but
  // setting only one size.
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;
  VerifyIconsMatch(last_bar_icon, action_icon);

  // Check that only the smaller size was set (only a 21px icon was provided).
  EXPECT_TRUE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_FALSE(action_icon.ToImageSkia()->HasRepresentation(kLargeIconScale));

  // Tell the extension to update the icon using dictionary of ImageData
  // objects, but setting only size 42.
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());

  EXPECT_FALSE(gfx::test::AreImagesEqual(last_bar_icon,
                                         GetBrowserActionsBar()->GetIcon(0)));
  last_bar_icon = GetBrowserActionsBar()->GetIcon(0);

  action_icon = icon_factory.GetIcon(0);
  action_icon_current_id = action_icon.ToSkBitmap()->getGenerationID();
  EXPECT_GT(action_icon_current_id, action_icon_last_id);
  action_icon_last_id = action_icon_current_id;

  // Check that only the larger size was set (only a 42px icon was provided).
  EXPECT_FALSE(action_icon.ToImageSkia()->HasRepresentation(kSmallIconScale));
  EXPECT_TRUE(action_icon.AsImageSkia().HasRepresentation(kLargeIconScale));

  // Try setting icon with empty dictionary of ImageData objects.
  GetBrowserActionsBar()->Press(0);
  ASSERT_FALSE(catcher.GetNextResult());
  EXPECT_EQ(kEmptyImageDataError, catcher.message());

  // Try setting icon with empty dictionary of path objects.
  GetBrowserActionsBar()->Press(0);
  ASSERT_FALSE(catcher.GetNextResult());
  EXPECT_EQ(kEmptyPathError, catcher.message());
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, TabSpecificBrowserActionState) {
  ASSERT_TRUE(RunExtensionTest("action/default_enabled/tab_specific_state"))
      << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Test that there is an action in the toolbar and that it has an icon.
  ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());
  EXPECT_TRUE(GetBrowserActionsBar()->HasIcon(0));

  // Execute the action, its title should change.
  ResultCatcher catcher;
  GetBrowserActionsBar()->Press(0);
  ASSERT_TRUE(catcher.GetNextResult());
  EXPECT_EQ("Showing icon 2", GetBrowserActionsBar()->GetTooltip(0));

  // Open a new tab, the title should go back.
  chrome::NewTab(browser());
  EXPECT_EQ("hi!", GetBrowserActionsBar()->GetTooltip(0));

  // Go back to first tab, changed title should reappear.
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_EQ("Showing icon 2", GetBrowserActionsBar()->GetTooltip(0));

  // Reload that tab, default title should come back.
  ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
  EXPECT_EQ("hi!", GetBrowserActionsBar()->GetTooltip(0));
}

// Test that calling chrome.action.setPopup() can enable a popup.
IN_PROC_BROWSER_TEST_P(ActionApiTestWithParam, AddPopup) {
  const bool kDefaultEnabled = GetParam();
  std::string default_state =
      kDefaultEnabled ? "default_enabled" : "default_disabled";
  // Load the extension, which has no default popup.
  ASSERT_TRUE(RunExtensionTest("action/" + default_state + "/add_popup"))
      << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  int tab_id = ExtensionTabUtil::GetTabId(
      browser()->tab_strip_model()->GetActiveWebContents());

  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action) << "Action test extension should have an action.";

  ASSERT_FALSE(action->HasPopup(tab_id));
  if (kDefaultEnabled)
    ASSERT_FALSE(action->HasPopup(ExtensionAction::kDefaultTabId));

  // Simulate the action being clicked.  The resulting event should
  // install action popup.
  {
    ResultCatcher catcher;
    ExecuteExtensionAction(browser(), extension);
    ASSERT_TRUE(catcher.GetNextResult());
  }

  ASSERT_TRUE(action->HasPopup(tab_id))
      << "Clicking on the action should have caused a popup to be added.";
  if (kDefaultEnabled) {
    ASSERT_FALSE(action->HasPopup(ExtensionAction::kDefaultTabId))
        << "Clicking on the action should not have set a default "
        << "popup.";
  }

  ASSERT_STREQ("/a_popup.html", action->GetPopupUrl(tab_id).path().c_str());

  // Now change the popup from a_popup.html to a_second_popup.html .
  // Load a page which removes the popup using chrome.pageAction.setPopup().
  {
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("change_popup.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  ASSERT_TRUE(action->HasPopup(tab_id));
  if (kDefaultEnabled)
    ASSERT_TRUE(action->HasPopup(ExtensionAction::kDefaultTabId));
  ASSERT_STREQ("/another_popup.html",
               action->GetPopupUrl(tab_id).path().c_str());
}

// Test that calling chrome.action.setPopup() can remove a popup.
IN_PROC_BROWSER_TEST_P(ActionApiTestWithParam, RemovePopup) {
  // Load the extension, which has an action with a default popup.
  const bool kDefaultEnabled = GetParam();
  std::string default_state =
      kDefaultEnabled ? "default_enabled" : "default_disabled";
  ASSERT_TRUE(RunExtensionTest("action/" + default_state + "/remove_popup"))
      << message_;

  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  int tab_id = ExtensionTabUtil::GetTabId(
      browser()->tab_strip_model()->GetActiveWebContents());

  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action) << "Action test extension should have an action.";

  ASSERT_TRUE(action->HasPopup(tab_id))
      << "Expect an action popup before the test removes it.";
  if (kDefaultEnabled) {
    ASSERT_TRUE(action->HasPopup(ExtensionAction::kDefaultTabId))
        << "Expect an action popup is the default for all tabs.";
  }

  // Load a page which removes the popup using chrome.browserAction.setPopup().
  {
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("remove_popup.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  ASSERT_FALSE(action->HasPopup(tab_id))
      << "Action popup should have been removed.";
  if (kDefaultEnabled) {
    ASSERT_TRUE(action->HasPopup(ExtensionAction::kDefaultTabId))
        << "Action popup default should not be changed by setting a specific "
           "tab "
        << "id.";
  }
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, IncognitoBasic) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ASSERT_TRUE(RunExtensionTest("action/default_enabled/basics")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Test that there is an action in the toolbar.
  ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());

  // Open an incognito window and test that the action isn't there by
  // default.
  Profile* incognito_profile = browser()->profile()->GetOffTheRecordProfile();
  base::RunLoop().RunUntilIdle();  // Wait for profile initialization.
  Browser* incognito_browser =
      new Browser(Browser::CreateParams(incognito_profile, true));

  ASSERT_EQ(0,
            BrowserActionTestUtil(incognito_browser).NumberOfBrowserActions());

  // Now enable the extension in incognito mode, and test that the browser
  // action shows up.
  // SetIsIncognitoEnabled() requires a reload of the extension, so we have to
  // wait for it.
  TestExtensionRegistryObserver registry_observer(
      ExtensionRegistry::Get(profile()), extension->id());
  extensions::util::SetIsIncognitoEnabled(extension->id(), browser()->profile(),
                                          true);
  registry_observer.WaitForExtensionLoaded();

  ASSERT_EQ(1,
            BrowserActionTestUtil(incognito_browser).NumberOfBrowserActions());

  // TODO(mpcomplete): simulate a click and have it do the right thing in
  // incognito.
}

// Tests that events are dispatched to the correct profile for split mode
// extensions.
IN_PROC_BROWSER_TEST_F(ActionApiTest, IncognitoSplit) {
  ResultCatcher catcher;
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("action/default_enabled/split_mode"),
      kFlagEnableIncognito);
  ASSERT_TRUE(extension) << message_;

  // Open an incognito window.
  Profile* incognito_profile = browser()->profile()->GetOffTheRecordProfile();
  Browser* incognito_browser =
      new Browser(Browser::CreateParams(incognito_profile, true));
  base::RunLoop().RunUntilIdle();  // Wait for profile initialization.
  // Navigate just to have a tab in this window, otherwise wonky things happen.
  OpenURLOffTheRecord(browser()->profile(), GURL("about:blank"));
  ASSERT_EQ(1,
            BrowserActionTestUtil(incognito_browser).NumberOfBrowserActions());

  // A click in the regular profile should open a tab in the regular profile.
  ExecuteExtensionAction(browser(), extension);
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  // A click in the incognito profile should open a tab in the
  // incognito profile.
  ExecuteExtensionAction(incognito_browser, extension);
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, BadgeBackgroundColor) {
  ASSERT_TRUE(embedded_test_server()->Start());
  ASSERT_TRUE(RunExtensionTest("action/default_enabled/color")) << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Test that there is an action in the toolbar.
  ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());

  // Test that CSS values (#FF0000) set color correctly.
  ExtensionAction* action = GetAction(*extension);
  ASSERT_EQ(SkColorSetARGB(255, 255, 0, 0),
            action->GetBadgeBackgroundColor(ExtensionAction::kDefaultTabId));

  // Tell the extension to update the action state.
  ResultCatcher catcher;
  ui_test_utils::NavigateToURL(browser(),
                               GURL(extension->GetResourceURL("update.html")));
  ASSERT_TRUE(catcher.GetNextResult());

  // Test that CSS values (#0F0) set color correctly.
  ASSERT_EQ(SkColorSetARGB(255, 0, 255, 0),
            action->GetBadgeBackgroundColor(ExtensionAction::kDefaultTabId));

  ui_test_utils::NavigateToURL(browser(),
                               GURL(extension->GetResourceURL("update2.html")));
  ASSERT_TRUE(catcher.GetNextResult());

  // Test that array values set color correctly.
  ASSERT_EQ(SkColorSetARGB(255, 255, 255, 255),
            action->GetBadgeBackgroundColor(ExtensionAction::kDefaultTabId));

  ui_test_utils::NavigateToURL(browser(),
                               GURL(extension->GetResourceURL("update3.html")));
  ASSERT_TRUE(catcher.GetNextResult());

  // Test that hsl() values 'hsl(120, 100%, 50%)' set color correctly.
  ASSERT_EQ(SkColorSetARGB(255, 0, 255, 0),
            action->GetBadgeBackgroundColor(ExtensionAction::kDefaultTabId));

  // Test basic color keyword set correctly.
  ui_test_utils::NavigateToURL(browser(),
                               GURL(extension->GetResourceURL("update4.html")));
  ASSERT_TRUE(catcher.GetNextResult());

  ASSERT_EQ(SkColorSetARGB(255, 0, 0, 255),
            action->GetBadgeBackgroundColor(ExtensionAction::kDefaultTabId));
}

IN_PROC_BROWSER_TEST_P(ActionApiTestWithParam, Getters) {
  const bool kDefaultEnabled = GetParam();
  std::string default_state =
      kDefaultEnabled ? "default_enabled" : "default_disabled";
  ASSERT_TRUE(RunExtensionTest("action/" + default_state + "/getters"))
      << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Test that there is an action in the toolbar.
  ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());

  // Test the getters for defaults.
  ResultCatcher catcher;
  ui_test_utils::NavigateToURL(browser(),
                               GURL(extension->GetResourceURL("update.html")));
  ASSERT_TRUE(catcher.GetNextResult());

  if (kDefaultEnabled) {
    // Test the getters for a specific tab.
    ui_test_utils::NavigateToURL(
        browser(), GURL(extension->GetResourceURL("update2.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }
}

// Verify triggering action.
// TODO (catmullings): The disabled state passes in production, but not in test.
IN_PROC_BROWSER_TEST_P(ActionApiTestWithParam, TestTriggerAction) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const bool kDefaultEnabled = GetParam();
  std::string default_state =
      kDefaultEnabled ? "default_enabled" : "default_disabled";
  ASSERT_TRUE(RunExtensionTest("trigger_actions/action/" + default_state))
      << message_;
  const Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Test that there is an action in the toolbar.
  ASSERT_EQ(1, GetBrowserActionsBar()->NumberOfBrowserActions());

  // For the default disabled action, its icon is displayed when a tab is
  // created.
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/simple.html"));

  if (!kDefaultEnabled) {
    chrome::NewTab(browser());
    browser()->tab_strip_model()->ActivateTabAt(0, true);

    // Give the extension time to show the action on the tab.
    // TODO(catmullings): This hangs.
    WaitForActionVisibilityChangeTo(1);
  }

  ExtensionAction* action = GetAction(*extension);
  ASSERT_TRUE(action);

  // Simulate a click on the action icon.
  {
    ResultCatcher catcher;
    if (kDefaultEnabled) {
      GetBrowserActionsBar()->Press(0);
    } else {
      ExtensionActionRunner::GetForWebContents(
          browser()->tab_strip_model()->GetActiveWebContents())
          ->RunAction(extension, true);
    }

    EXPECT_TRUE(catcher.GetNextResult());
  }

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(tab != NULL);

  if (!kDefaultEnabled)
    EXPECT_TRUE(action->GetIsVisible(ExtensionTabUtil::GetTabId(tab)));

  // Verify that the action turned the background color red.
  const std::string script =
      "window.domAutomationController.send(document.body.style."
      "backgroundColor);";
  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(tab, script, &result));
  EXPECT_EQ(result, "red");
}

IN_PROC_BROWSER_TEST_F(ActionApiTest, DISABLED_ActionWithRectangularIcon) {
  ExtensionTestMessageListener ready_listener("ready", true);
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("browser_action").AppendASCII("rect_icon")));
  EXPECT_TRUE(ready_listener.WaitUntilSatisfied());
  gfx::Image first_icon = GetBrowserActionsBar()->GetIcon(0);
  ResultCatcher catcher;
  ready_listener.Reply(std::string());
  EXPECT_TRUE(catcher.GetNextResult());
  gfx::Image next_icon = GetBrowserActionsBar()->GetIcon(0);
  EXPECT_FALSE(gfx::test::AreImagesEqual(first_icon, next_icon));
}

}  // namespace
}  // namespace extensions
