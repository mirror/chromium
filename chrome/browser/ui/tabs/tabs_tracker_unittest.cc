// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tabs_tracker.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_logger.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/test/test_web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::TestWebContents;
using testing::StrictMock;

namespace tabs {
namespace {

class MockTabLogger : public TabLogger {
 public:
  MOCK_METHOD4(LogBackgroundTab,
               void(const content::WebContents* web_contents,
                    const Browser* browser,
                    const TabStripModel* tab_strip_model,
                    int index));
};

}  // namespace

// Inherits from ChromeRenderViewHostTestHarness to use TestWebContents and
// Profile.
class TabsTrackerTest : public ChromeRenderViewHostTestHarness {
 public:
  TabsTrackerTest() = default;
  ~TabsTrackerTest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    // Create and pass a unique_ptr, but retain a pointer to the object.
    {
      auto mock_logger_transient =
          std::make_unique<StrictMock<MockTabLogger>>();
      mock_logger_ = mock_logger_transient.get();
      g_browser_process->GetTabsTracker()->SetTabLoggerForTest(
          std::move(mock_logger_transient));
    }
  }

  void TearDown() override {
    tabs_tracker_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  StrictMock<MockTabLogger>* mock_logger() { return mock_logger_; }

 private:
  std::unique_ptr<TabsTracker> tabs_tracker_;
  StrictMock<MockTabLogger>* mock_logger_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TabsTrackerTest);
};

TEST_F(TabsTrackerTest, Basic) {
  Browser::CreateParams params(profile(), true);
  auto browser = CreateBrowserWithTestWindowForParams(&params);

  TestWebContents* test_contents = TestWebContents::Create(profile(), nullptr);
  TestWebContents* test_contents2 = TestWebContents::Create(profile(), nullptr);

  TabStripModel* tab_strip = browser->tab_strip_model();
  tab_strip->AppendWebContents(test_contents, false);
  tab_strip->AppendWebContents(test_contents2, false);

  // Start with the leftmost tab activated.
  tab_strip->ActivateTabAt(0, false);

  // Pinning or unpinning test_contents2 triggers the log.
  // Note that pinning the tab moves it to index 0.
  EXPECT_CALL(*mock_logger(),
              LogBackgroundTab(test_contents2, browser.get(), tab_strip, 0))
      .Times(1);
  tab_strip->SetTabPinned(1, true);

  EXPECT_CALL(*mock_logger(),
              LogBackgroundTab(test_contents2, browser.get(), tab_strip, 0))
      .Times(1);
  tab_strip->SetTabPinned(0, false);

  // Activate test_contents2 to deactivate the current tab.
  EXPECT_CALL(*mock_logger(),
              LogBackgroundTab(test_contents, browser.get(), tab_strip, 1))
      .Times(1);
  tab_strip->ActivateTabAt(0, true);
  test_contents->WasHidden();

  tab_strip->CloseAllTabs();
}

}  // namespace tabs
