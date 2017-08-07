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
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_tester.h"
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
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/web_contents_tester.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

using RestoredTab = SessionRestoreDelegate::RestoredTab;

class SessionRestorePageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        base::MakeUnique<SessionRestorePageLoadMetricsObserver>());
  }

 protected:
  SessionRestorePageLoadMetricsObserverTest() {}

  void SetUp() override {
    page_load_metrics::PageLoadMetricsObserverTestHarness::SetUp();

    // Clear committed URL for the next session restore starts from an empty
    // URL.
    NavigateAndCommit(web_contents(), GURL());
    StopLoading(web_contents());

    // Create the tab manager to register its SessionRestoreObserver before
    // session restore starts.
    g_browser_process->GetTabManager();

    restored_tabs_.emplace_back(web_contents(), false /* is_active */,
                                false /* is_app */, false /* is_pinned */);
    PopulateFirstPaintTimings();
  }

  void TearDown() override {
    page_load_metrics::PageLoadMetricsObserverTestHarness::TearDown();
  }

  // Populate first paint and first [contentful,meaningful] paint timings.
  void PopulateFirstPaintTimings() {
    page_load_metrics::InitPageLoadTimingForTest(&timing_);
    timing_.navigation_start = base::Time::FromDoubleT(1);
    timing_.paint_timing->first_meaningful_paint =
        base::TimeDelta::FromMilliseconds(10);
    PopulateRequiredTimingFields(&timing_);
  }

  void ExpectFirstPaintMetricsTotalCount(int expected_total_count) const {
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

  void StopLoading(content::WebContents* contents) const {
    contents->Stop();
    content::WebContentsTester::For(contents)->TestSetIsLoading(false);
  }

  void NavigateAndCommit(content::WebContents* contents,
                         const GURL& url) const {
    content::NavigationSimulator::NavigateAndCommitFromDocument(
        GetTestURL(), contents->GetMainFrame());
  }

  GURL GetTestURL() const { return GURL("https://google.com"); }

  const page_load_metrics::mojom::PageLoadTiming& timing() const {
    return timing_;
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
  NavigateAndCommit(web_contents(), GetTestURL());
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest, RestoreSingleForegroundTab) {
  // Restore one tab which finishes loading in foreground.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommit(web_contents(), GetTestURL());
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(1);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       RestoreMultipleForegroundTabs) {
  std::unique_ptr<content::WebContents> test_contents(CreateTestWebContents());
  std::unique_ptr<page_load_metrics::PageLoadMetricsObserverTester> tester(
      new page_load_metrics::PageLoadMetricsObserverTester(
          test_contents.get(),
          base::BindRepeating(
              &SessionRestorePageLoadMetricsObserverTest::RegisterObservers,
              base::Unretained(this))));
  test_contents->WasShown();
  std::vector<RestoredTab> restored_tabs{
      RestoredTab(test_contents.get(), false, false, false)};

  // Simulate first paints for test_contents.
  SessionRestore::OnWillRestoreTab(test_contents.get());
  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());
  NavigateAndCommit(test_contents.get(), GetTestURL());
  tester->SimulateTimingUpdate(timing());
  ExpectFirstPaintMetricsTotalCount(1);

  // Simulate first paints for web_contents().
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommit(web_contents(), GetTestURL());
  SimulateTimingUpdate();
  ExpectFirstPaintMetricsTotalCount(2);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest, RestoreBackgroundTab) {
  // Set the tab to background before the PageLoadMetricsObserver was created.
  web_contents()->WasHidden();

  // Load the restored tab in background.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommit(web_contents(), GetTestURL());
  SimulateTimingUpdate();

  // No paint timings recorded for tabs restored in background.
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest, HideTabBeforeFirstPaints) {
  // Start loading the tab.
  SessionRestore::OnWillRestoreTab(web_contents());
  RestoreTabs();
  NavigateAndCommit(web_contents(), GetTestURL());

  // Hide the tab before any paints.
  web_contents()->WasHidden();
  SimulateTimingUpdate();

  // No paint timings recorded because tab was hidden before the first paints.
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest,
       SwitchInitialRestoredForegroundTab) {
  std::unique_ptr<content::WebContents> test_contents(CreateTestWebContents());
  std::unique_ptr<page_load_metrics::PageLoadMetricsObserverTester> tester(
      new page_load_metrics::PageLoadMetricsObserverTester(
          test_contents.get(),
          base::BindRepeating(
              &SessionRestorePageLoadMetricsObserverTest::RegisterObservers,
              base::Unretained(this))));

  std::vector<RestoredTab> restored_tabs{
      RestoredTab(web_contents(), false, false, false),
      RestoredTab(test_contents.get(), false, false, false)};

  // Create 2 tabs: tab 0 is foreground, tab 1 is background.
  restored_tabs[0].contents()->WasShown();
  restored_tabs[1].contents()->WasHidden();

  // Restore both tabs.
  SessionRestore::OnWillRestoreTab(restored_tabs[0].contents());
  SessionRestore::OnWillRestoreTab(restored_tabs[1].contents());
  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());

  NavigateAndCommit(restored_tabs[0].contents(), GetTestURL());
  NavigateAndCommit(restored_tabs[1].contents(), GetTestURL());

  // Switch to tab 1 before any paint events occur.
  restored_tabs[0].contents()->WasHidden();
  restored_tabs[1].contents()->WasShown();
  SimulateTimingUpdate();
  tester->SimulateTimingUpdate(timing());

  // No paint timings recorded because the initial foreground tab was hidden.
  ExpectFirstPaintMetricsTotalCount(0);
}

TEST_F(SessionRestorePageLoadMetricsObserverTest, MultipleSessionRestores) {
  size_t number_of_session_restores = 3;
  for (size_t i = 0; i < number_of_session_restores; ++i) {
    // Restore session.
    SessionRestore::OnWillRestoreTab(web_contents());
    RestoreTabs();
    NavigateAndCommit(web_contents(), GetTestURL());
    SimulateTimingUpdate();

    // Number of paint timings should match the number of session restores.
    ExpectFirstPaintMetricsTotalCount(i + 1);

    // Clear committed URL for the next restore starts from an empty URL.
    StopLoading(web_contents());
    NavigateAndCommit(web_contents(), GURL());
    StopLoading(web_contents());
  }
}
