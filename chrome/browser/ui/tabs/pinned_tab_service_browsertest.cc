// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/pinned_tab_codec.h"
#include "chrome/browser/ui/tabs/pinned_tab_service.h"
#include "chrome/browser/ui/tabs/pinned_tab_service_factory.h"
#include "chrome/browser/ui/tabs/pinned_tab_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"

namespace {

class ClosingCounter : public BrowserListObserver {
 public:
  ClosingCounter() { BrowserList::AddObserver(this); }

  ~ClosingCounter() override { BrowserList::RemoveObserver(this); }

  int closing_count() const { return closing_count_; }
  int removed_count() const { return removed_count_; }

 private:
  // BrowserListObserver override
  void OnBrowserClosing(Browser* browser) override { ++closing_count_; }

  void OnBrowserRemoved(Browser* browser) override { ++removed_count_; }

  int closing_count_ = 0;
  int removed_count_ = 0;
};

class PinnedTabServiceBrowserTest : public InProcessBrowserTest {};

}  // namespace

// Makes sure pinned tabs are updated when tabstrip is empty.
// http://crbug.com/71939
IN_PROC_BROWSER_TEST_F(PinnedTabServiceBrowserTest, TabStripEmpty) {
  Profile* profile = browser()->profile();
  EXPECT_TRUE(PinnedTabServiceFactory::GetForProfile(profile));
  EXPECT_TRUE(profile->GetPrefs());

  GURL url("http://www.google.com");
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_TYPED);
  ui_test_utils::NavigateToURL(&params);

  TabStripModel* tab_strip_model = browser()->tab_strip_model();
  tab_strip_model->SetTabPinned(0, true);

  PinnedTabCodec::WritePinnedTabs(profile);
  std::string result =
      PinnedTabTestUtils::TabsToString(PinnedTabCodec::ReadPinnedTabs(profile));
  EXPECT_EQ("http://www.google.com/:pinned", result);

  // When tab strip is empty, browser window will be closed and PinnedTabService
  // must update data on this event.
  // Also we want to check if OnBrowserClosing is called only once
  // to avoid redundant work.
  ClosingCounter closing_counter;
  content::WindowedNotificationObserver closed_observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  tab_strip_model->SetTabPinned(0, false);
  EXPECT_TRUE(
      tab_strip_model->CloseWebContentsAt(0, TabStripModel::CLOSE_NONE));
  EXPECT_TRUE(tab_strip_model->empty());
  closed_observer.Wait();

  // Closing won't be notified. But we have fallback in Removed so
  // that PinnedTabService can do it's job.
  EXPECT_EQ(0, closing_counter.closing_count());
  EXPECT_EQ(1, closing_counter.removed_count());

  // Let's see it's cleared out properly.
  result =
      PinnedTabTestUtils::TabsToString(PinnedTabCodec::ReadPinnedTabs(profile));
  EXPECT_TRUE(result.empty());
}

IN_PROC_BROWSER_TEST_F(PinnedTabServiceBrowserTest, CloseWindow) {
  Profile* profile = browser()->profile();
  EXPECT_TRUE(PinnedTabServiceFactory::GetForProfile(profile));
  EXPECT_TRUE(profile->GetPrefs());

  GURL url("http://www.google.com");
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_TYPED);
  ui_test_utils::NavigateToURL(&params);

  TabStripModel* tab_strip_model = browser()->tab_strip_model();
  tab_strip_model->SetTabPinned(0, true);

  ClosingCounter closing_counter;
  content::WindowedNotificationObserver closed_observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  browser()->window()->Close();
  closed_observer.Wait();

  // Note that |closing_count| can be bigger than 1. eg. It's called
  // by BrowserView::CanClose() and then by
  // UnloadController::ProcessPendingTabs.
  EXPECT_GT(closing_counter.closing_count(), 1);
  EXPECT_EQ(1, closing_counter.removed_count());

  std::string result =
      PinnedTabTestUtils::TabsToString(PinnedTabCodec::ReadPinnedTabs(profile));
  EXPECT_EQ("http://www.google.com/:pinned", result);
}
