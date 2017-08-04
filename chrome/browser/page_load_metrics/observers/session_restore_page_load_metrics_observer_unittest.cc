// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/session_restore_page_load_metrics_observer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/sessions/tab_loader.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "content/public/test/web_contents_tester.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

using RestoredTab = SessionRestoreDelegate::RestoredTab;

namespace {

const char kDefaultTestUrl[] = "https://google.com";
const char kDefaultTestUrl2[] = "https://example.com";

}  // namespace

class SessionRestorePageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 protected:
  SessionRestorePageLoadMetricsObserverTest() {}

  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    std::unique_ptr<SessionRestorePageLoadMetricsObserver> observer =
        base::MakeUnique<SessionRestorePageLoadMetricsObserver>();
    if (observer)
      tracker->AddObserver(std::move(observer));
  }

  void SetUp() override {
    page_load_metrics::PageLoadMetricsObserverTestHarness::SetUp();

    // Stop the initial loading.
    StopLoading(web_contents());

    // Ensure the web_contents is in the foreground.
    web_contents()->WasShown();

    // Create the tab manager to register its SessionRestoreObserver before
    // session restore starts.
    g_browser_process->GetTabManager();

    restored_tabs_.emplace_back(web_contents(), false /* is_active */,
                                false /* is_app */, false /* is_pinned */);
    PopulateAllFirstPaintTimings(&timing_);
  }

  void TearDown() override {
    page_load_metrics::PageLoadMetricsObserverTestHarness::TearDown();
  }

  // Populate first paint and first [contentful,meaningful] paint timings.
  void PopulateAllFirstPaintTimings(
      page_load_metrics::mojom::PageLoadTiming* timing_) {
    page_load_metrics::InitPageLoadTimingForTest(timing_);
    timing_->navigation_start = base::Time::FromDoubleT(1);
    timing_->paint_timing->first_meaningful_paint =
        base::TimeDelta::FromMilliseconds(10);
    PopulateRequiredTimingFields(timing_);
  }

  void ExpectFirstPaintMetricsTotalCount(int expected_total_count) {
    histogram_tester().ExpectTotalCount(
        internal::kHistogramSessionRestoreForegroundTabFirstPaint,
        expected_total_count);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramSessionRestoreForegroundTabFirstContentfulPaint,
        expected_total_count);
    histogram_tester().ExpectTotalCount(
        internal::kHistogramSessionRestoreForegroundTabFirstMeaningfulPaint,
        expected_total_count);
  }

  void RestoreTabs() {
    TabLoader::RestoreTabs(restored_tabs_, base::TimeTicks());
  }

  void SimulateTimingUpdate() {
    PageLoadMetricsObserverTestHarness::SimulateTimingUpdate(timing_);
  }

  void StopLoading(content::WebContents* contents) {
    contents->Stop();
    content::WebContentsTester::For(contents)->TestSetIsLoading(false);
  }

  // Navigate with a RELOAD page transition to emulate session restore.
  void NavigateAndCommitWithReloadTransition(content::WebContents* contents,
                                             const GURL& url) {
    contents->GetController().LoadURL(
        url, content::Referrer(), ui::PAGE_TRANSITION_RELOAD, std::string());
    GURL loaded_url(url);
    bool reverse_on_redirect = false;
    content::BrowserURLHandler::GetInstance()->RewriteURLIfNecessary(
        &loaded_url, contents->GetBrowserContext(), &reverse_on_redirect);
    // LoadURL created a navigation entry, now simulate the RenderView sending
    // a notification that it actually navigated.
    content::WebContentsTester::For(contents)->CommitPendingNavigation();
  }

 private:
  page_load_metrics::mojom::PageLoadTiming timing_;
  std::vector<RestoredTab> restored_tabs_;

  DISALLOW_COPY_AND_ASSIGN(SessionRestorePageLoadMetricsObserverTest);
};

TEST_F(SessionRestorePageLoadMetricsObserverTest, NoMetrics) {
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       FirstPaintsOutOfSessionRestore) {
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl));
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(0);
}

// TODO(ducbui): Should observe multiple tabs.
TEST_F(SessionRestorePageLoadMetricsObserverTest, InitialForegroundTabChanged) {
  std::unique_ptr<content::WebContents> test_contents(CreateTestWebContents());
  // Create 2 tabs: tab 0 is foreground, tab 1 is background.
  test_contents.get()->WasShown();
  web_contents()->WasHidden();

  std::vector<RestoredTab> restored_tabs{
      RestoredTab(test_contents.get(), false, false, false),
      RestoredTab(web_contents(), false, false, false)};

  // Start both tabs but switch them before any paint events occur.
  SessionRestore::OnWillRestoreTab(restored_tabs[0].contents());
  SessionRestore::OnWillRestoreTab(restored_tabs[1].contents());
  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());
  NavigateAndCommitWithReloadTransition(restored_tabs[0].contents(),
                                        GURL("http://1"));
  NavigateAndCommitWithReloadTransition(restored_tabs[1].contents(),
                                        GURL(kDefaultTestUrl));

  // Switch to tab 1 and load it completely.
  restored_tabs[0].contents()->WasHidden();
  restored_tabs[1].contents()->WasShown();
  SimulateTimingUpdate();

  // No paint timings recorded because the initial foreground tab was changed.
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       ForegroundTabLoadedInSessionRestore) {
  // Restore one tab which finishes loading in foreground.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl));
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(1);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       BackgroundTabLoadedInSessionRestore) {
  // Set the tab to background before the PageLoadMetricsObserver was created.
  web_contents()->WasHidden();

  // Load the restored tab in background.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl));
  SimulateTimingUpdate();

  // No paint timings recorded because the loaded tab was in background.
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       TabIsHiddenBeforeFirstPaints) {
  // Hide the tab after it starts loading but before it finishes loading.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl));
  web_contents()->WasHidden();
  SimulateTimingUpdate();

  // No paint timings recorded because tab was hidden before the first paints.
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       MultipleSessionRestoresWithoutInitialForegroundTabChange) {
  // First session restore.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl));
  SimulateTimingUpdate();
  StopLoading(web_contents());
  ExpectFirstPaintMetricsTotalCount(1);

  // Clear committed URL so the next session restore starts from an empty URL.
  NavigateAndCommitWithReloadTransition(web_contents(), GURL());
  StopLoading(web_contents());

  // Second session restore.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();

  // Load the only foreground tab with first paints.
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl2));
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(2);
}

// TODO(ducbui): Should observe multiple tabs.
TEST_F(SessionRestorePageLoadMetricsObserverTest,
       MultipleSessionRestoresWithInitialForegroundTabChange) {
  std::unique_ptr<content::WebContents> test_contents(CreateTestWebContents());
  std::vector<RestoredTab> restored_tabs{
      RestoredTab(test_contents.get(), false, false, false),
      RestoredTab(web_contents(), false, false, false)};

  // Tab strip with 2 tabs: tab 0 is foreground, tab 1 is background.
  restored_tabs[0].contents()->WasShown();
  restored_tabs[1].contents()->WasHidden();

  // First session restore.
  SessionRestore::OnWillRestoreTab(restored_tabs[0].contents());
  SessionRestore::OnWillRestoreTab(restored_tabs[1].contents());
  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());

  // Start and stop loading foreground tab 0 before its first paints.
  NavigateAndCommitWithReloadTransition(restored_tabs[0].contents(),
                                        GURL(kDefaultTestUrl));
  StopLoading(restored_tabs[0].contents());

  // Switch to tab 1 to change the initial foreground tab.
  test_contents.get()->WasHidden();
  web_contents()->WasShown();

  // Load the foreground tab with first paints.
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl));
  SimulateTimingUpdate();
  StopLoading(web_contents());

  // No paint timings recorded because the initial foreground tab was changed.
  ExpectFirstPaintMetricsTotalCount(0);

  // Clear committed URL so the next session restore starts from an empty URL.
  NavigateAndCommitWithReloadTransition(web_contents(), GURL());
  StopLoading(web_contents());

  // Second session restore.
  SessionRestore::OnWillRestoreTab(restored_tabs[0].contents());
  SessionRestore::OnWillRestoreTab(restored_tabs[1].contents());
  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());

  // Load the foreground tab with first paints.
  NavigateAndCommitWithReloadTransition(web_contents(), GURL(kDefaultTestUrl2));
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(1);
}
