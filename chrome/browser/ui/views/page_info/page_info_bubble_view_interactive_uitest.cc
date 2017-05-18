// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "content/public/test/browser_test_utils.h"

namespace test {

typedef InProcessBrowserTest PageInfoBubbleViewBrowserTest;

namespace {

// Clicks the location icon to open the page info bubble.
void OpenPageInfoBubble(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  views::View* location_icon_view =
      browser_view->toolbar()->location_bar()->location_icon_view();
  ASSERT_TRUE(location_icon_view);

  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  ui_test_utils::MoveMouseToCenterAndPress(
      location_icon_view, ui_controls::LEFT,
      ui_controls::DOWN | ui_controls::UP, runner->QuitClosure());
  runner->Run();
}

// Opens the Page Info bubble, clicks the "Site settings" button, then closes
// the bubble, returning the button.
views::View* GetSiteSettingsButton(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  std::set<views::Widget*> child_widgets_before;
  std::set<views::Widget*> child_widgets_after;
  // Compare the list of Browser child Widgets before and after opening the Page
  // Info bubble to identify which Widget it is.
  views::Widget::GetAllOwnedWidgets(browser_view->GetWidget()->GetNativeView(),
                                    &child_widgets_before);

  OpenPageInfoBubble(browser);
  views::Widget::GetAllOwnedWidgets(browser_view->GetWidget()->GetNativeView(),
                                    &child_widgets_after);

  // Check this caused one additional Widget to appear (the Page Info bubble).
  views::Widget* page_info_bubble = nullptr;
  EXPECT_EQ(child_widgets_before.size() + 1, child_widgets_after.size());
  for (auto it = child_widgets_after.begin(); it != child_widgets_after.end();
       ++it) {
    if (child_widgets_before.count(*it) == 0)
      page_info_bubble = *it;
  }
  EXPECT_TRUE(page_info_bubble);

  // Retrieve the "Site settings" button.
  views::View* site_settings_button =
      page_info_bubble->GetRootView()->GetViewByID(LINK_SITE_SETTINGS);
  EXPECT_TRUE(site_settings_button);
  return site_settings_button;
}

void ClickAndWaitForSettingsPageToOpen(views::View* site_settings_button) {
  content::WebContentsAddedObserver new_tab_observer;
  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner(
          content::MessageLoopRunner::QuitMode::DEFERRED);
  ui_test_utils::MoveMouseToCenterAndPress(
      site_settings_button, ui_controls::LEFT,
      ui_controls::DOWN | ui_controls::UP, runner->QuitClosure());

  base::string16 expected_title(base::ASCIIToUTF16("Settings"));
  content::TitleWatcher title_watcher(new_tab_observer.GetWebContents(),
                                      expected_title);
  EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest, ShowBubble) {
  OpenPageInfoBubble(browser());
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_PAGE_INFO,
            PageInfoBubbleView::GetShownBubbleType());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest, ChromeURL) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings"));
  OpenPageInfoBubble(browser());
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_INTERNAL_PAGE,
            PageInfoBubbleView::GetShownBubbleType());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest, ChromeExtensionURL) {
  ui_test_utils::NavigateToURL(
      browser(), GURL("chrome-extension://extension-id/options.html"));
  OpenPageInfoBubble(browser());
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_INTERNAL_PAGE,
            PageInfoBubbleView::GetShownBubbleType());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest, ChromeDevtoolsURL) {
  ui_test_utils::NavigateToURL(
      browser(), GURL("chrome-devtools://devtools/bundled/inspector.html"));
  OpenPageInfoBubble(browser());
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_INTERNAL_PAGE,
            PageInfoBubbleView::GetShownBubbleType());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest, ViewSourceURL) {
  ui_test_utils::NavigateToURL(browser(), GURL(url::kAboutBlankURL));
  chrome::ViewSelectedSource(browser());
  OpenPageInfoBubble(browser());
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_INTERNAL_PAGE,
            PageInfoBubbleView::GetShownBubbleType());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest, SiteSettingsLink) {
  ui_test_utils::NavigateToURL(browser(), GURL("https://www.google.com/"));
  views::View* site_settings_button = GetSiteSettingsButton(browser());

  ClickAndWaitForSettingsPageToOpen(site_settings_button);
  EXPECT_EQ(chrome::kChromeUIContentSettingsURL, browser()
                                                     ->tab_strip_model()
                                                     ->GetActiveWebContents()
                                                     ->GetLastCommittedURL()
                                                     .spec());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest,
                       SiteSettingsLinkWithSiteDetailsEnabled) {
  ui_test_utils::NavigateToURL(browser(), GURL("https://www.google.com/"));
  views::View* site_settings_button = GetSiteSettingsButton(browser());
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kSiteDetails);

  ClickAndWaitForSettingsPageToOpen(site_settings_button);
  std::string expected_origin = "https%3A%2F%2Fwww.google.com%2F";
  EXPECT_EQ(chrome::kChromeUISiteDetailsPrefixURL + expected_origin,
            browser()
                ->tab_strip_model()
                ->GetActiveWebContents()
                ->GetLastCommittedURL()
                .spec());
}

IN_PROC_BROWSER_TEST_F(PageInfoBubbleViewBrowserTest,
                       SiteSettingsLinkAboutBlankWithSiteDetailsEnabled) {
  ui_test_utils::NavigateToURL(browser(), GURL(url::kAboutBlankURL));
  views::View* site_settings_button = GetSiteSettingsButton(browser());
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kSiteDetails);
  ClickAndWaitForSettingsPageToOpen(site_settings_button);

  EXPECT_EQ(chrome::kChromeUIContentSettingsURL, browser()
                                                     ->tab_strip_model()
                                                     ->GetActiveWebContents()
                                                     ->GetLastCommittedURL()
                                                     .spec());
}

}  // namespace

}  // namespace test
