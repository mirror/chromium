// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_activity_watcher.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_snapshot_logger.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::WebContentsTester;
using testing::_;
using testing::StrictMock;

namespace {

class MockTabSnapshotLogger : public TabSnapshotLogger {
 public:
  MockTabSnapshotLogger() = default;

  // TabSnapshotLogger:
  MOCK_METHOD2(LogBackgroundTab,
               void(ukm::SourceId ukm_source_id,
                    content::WebContents* web_contents));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTabSnapshotLogger);
};

}  // namespace

// Inherits from ChromeRenderViewHostTestHarness to use TestWebContents and
// Profile.
class TabActivityWatcherTest : public ChromeRenderViewHostTestHarness {
 public:
  TabActivityWatcherTest() {
    // Create and pass a unique_ptr, but retain a pointer to the object.
    auto mock_logger_transient =
        std::make_unique<StrictMock<MockTabSnapshotLogger>>();
    mock_logger_ = mock_logger_transient.get();
    TabActivityWatcher::GetInstance()->SetTabSnapshotLoggerForTest(
        std::move(mock_logger_transient));
  }

  ~TabActivityWatcherTest() override {
    TabActivityWatcher::GetInstance()->SetTabSnapshotLoggerForTest(nullptr);
  }

 protected:
  StrictMock<MockTabSnapshotLogger>* mock_logger() { return mock_logger_; }

 private:
  StrictMock<MockTabSnapshotLogger>* mock_logger_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TabActivityWatcherTest);
};

TEST_F(TabActivityWatcherTest, Basic) {
  Browser::CreateParams params(profile(), true);
  auto browser = CreateBrowserWithTestWindowForParams(&params);

  content::WebContents* test_contents_1 =
      WebContentsTester::CreateTestWebContents(profile(), nullptr);
  content::WebContents* test_contents_2 =
      WebContentsTester::CreateTestWebContents(profile(), nullptr);

  TabStripModel* tab_strip = browser->tab_strip_model();
  tab_strip->AppendWebContents(test_contents_1, false);
  tab_strip->AppendWebContents(test_contents_2, false);

  // Start with the leftmost tab activated.
  tab_strip->ActivateTabAt(0, false);

  WebContentsTester::For(test_contents_1)
      ->NavigateAndCommit(GURL("https://example.com"));
  WebContentsTester::For(test_contents_2)
      ->NavigateAndCommit(GURL("https://example.com/2"));

  // Pinning or unpinning test_contents_2 triggers the log.
  // Note that pinning the tab moves it to index 0.
  EXPECT_CALL(*mock_logger(), LogBackgroundTab(_, test_contents_2));
  tab_strip->SetTabPinned(1, true);

  EXPECT_CALL(*mock_logger(), LogBackgroundTab(_, test_contents_2));
  tab_strip->SetTabPinned(0, false);

  // Activate test_contents_2 to deactivate the current tab.
  EXPECT_CALL(*mock_logger(), LogBackgroundTab(_, test_contents_1));
  tab_strip->ActivateTabAt(0, true);
  test_contents_1->WasHidden();

  tab_strip->CloseAllTabs();
}
