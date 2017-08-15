// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/test/test_web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::WebContents;

namespace resource_coordinator {

class TabManagerStatsCollectorTest : public ChromeRenderViewHostTestHarness {
 public:
  TabManagerStatsCollectorTest() = default;
  ~TabManagerStatsCollectorTest() override = default;

  WebContents* CreateWebContents() {
    return content::TestWebContents::Create(profile(), nullptr);
  }

  TabManagerStatsCollector* GetTabManagerStatsCollector() {
    return tab_manager_.tab_manager_stats_collector_.get();
  }

  void StartSessionRestore() {
    tab_manager_.OnSessionRestoreStartedLoadingTabs();
    GetTabManagerStatsCollector()->OnSessionRestoreStartedLoadingTabs();
  }

  void FinishSessionRestore() {
    tab_manager_.OnSessionRestoreFinishedLoadingTabs();
    GetTabManagerStatsCollector()->OnSessionRestoreFinishedLoadingTabs();
  }

  TabManager::WebContentsData* GetWebContentsData(WebContents* contents) const {
    return tab_manager_.GetWebContentsData(contents);
  }

 protected:
  TabManager tab_manager_;
};

class TabManagerStatsCollectorTabSwitchTest
    : public TabManagerStatsCollectorTest {
 public:
  TabManagerStatsCollectorTabSwitchTest() = default;
  ~TabManagerStatsCollectorTabSwitchTest() override = default;

  void SetForegroundTabLoadingState(TabLoadingState state) {
    GetWebContentsData(foreground_tab_)->SetTabLoadingState(state);
  }

  void SetBackgroundTabLoadingState(TabLoadingState state) {
    GetWebContentsData(background_tab_)->SetTabLoadingState(state);
  }

  // Creating and destroying the WebContentses need to be done in the test
  // scope.
  void SetForegroundAndBackgroundTabs(WebContents* foreground_tab,
                                      WebContents* background_tab) {
    foreground_tab_ = foreground_tab;
    background_tab_ = background_tab;
  }

  void FinishLoadingForegroundTab() {
    SetForegroundTabLoadingState(TAB_IS_LOADED);
    GetTabManagerStatsCollector()->OnDidStopLoading(foreground_tab_);
  }

  void FinishLoadingBackgroundTab() {
    SetBackgroundTabLoadingState(TAB_IS_LOADED);
    GetTabManagerStatsCollector()->OnDidStopLoading(background_tab_);
  }

  void SwitchToBackgroundTab() {
    GetTabManagerStatsCollector()->RecordSwitchToTab(foreground_tab_,
                                                     background_tab_);
    std::swap(foreground_tab_, background_tab_);
  }

 protected:
  WebContents* foreground_tab_;
  WebContents* background_tab_;
};

TEST_F(TabManagerStatsCollectorTabSwitchTest,
       HistogramsSessionRestoreSwitchToTab) {
  const char kHistogramName[] = "TabManager.SessionRestore.SwitchToTab";
  base::HistogramTester histograms;
  histograms.ExpectTotalCount(kHistogramName, 0);

  std::unique_ptr<WebContents> tab1(CreateWebContents());
  std::unique_ptr<WebContents> tab2(CreateWebContents());
  SetForegroundAndBackgroundTabs(tab1.get(), tab2.get());

  SetForegroundTabLoadingState(TAB_IS_LOADING);
  SetBackgroundTabLoadingState(TAB_IS_LOADING);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  // Nothing should happen until we're in a session restore.
  histograms.ExpectTotalCount(kHistogramName, 0);

  StartSessionRestore();
  SetBackgroundTabLoadingState(TAB_IS_NOT_LOADING);
  SetForegroundTabLoadingState(TAB_IS_NOT_LOADING);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  histograms.ExpectTotalCount(kHistogramName, 2);
  histograms.ExpectBucketCount(kHistogramName, TAB_IS_NOT_LOADING, 2);

  SetBackgroundTabLoadingState(TAB_IS_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADING);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  histograms.ExpectTotalCount(kHistogramName, 5);
  histograms.ExpectBucketCount(kHistogramName, TAB_IS_NOT_LOADING, 2);
  histograms.ExpectBucketCount(kHistogramName, TAB_IS_LOADING, 3);

  SetBackgroundTabLoadingState(TAB_IS_LOADED);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  histograms.ExpectTotalCount(kHistogramName, 9);
  histograms.ExpectBucketCount(kHistogramName, TAB_IS_NOT_LOADING, 2);
  histograms.ExpectBucketCount(kHistogramName, TAB_IS_LOADING, 3);
  histograms.ExpectBucketCount(kHistogramName, TAB_IS_LOADED, 4);
}

TEST_F(TabManagerStatsCollectorTabSwitchTest,
       HistogramsSessionRestoreTabSwitchLoadTime) {
  const char kHistogramName[] =
      "TabManager.Experimental.SessionRestore.TabSwitchLoadTime";
  base::HistogramTester histograms;

  std::unique_ptr<WebContents> tab1(CreateWebContents());
  std::unique_ptr<WebContents> tab2(CreateWebContents());
  SetForegroundAndBackgroundTabs(tab1.get(), tab2.get());
  StartSessionRestore();

  SetBackgroundTabLoadingState(TAB_IS_NOT_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  FinishLoadingForegroundTab();
  histograms.ExpectTotalCount(kHistogramName, 1);
  SetBackgroundTabLoadingState(TAB_IS_LOADING);
  SwitchToBackgroundTab();
  FinishLoadingForegroundTab();
  histograms.ExpectTotalCount(kHistogramName, 2);

  // Metrics aren't recorded when the foreground tab has not finished loading
  // and the user switches to a different tab.
  SetBackgroundTabLoadingState(TAB_IS_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  // Foreground tab is currently loading and being tracked.
  SwitchToBackgroundTab();
  // The previous foreground tab is no longer tracked.
  FinishLoadingBackgroundTab();
  SwitchToBackgroundTab();
  histograms.ExpectTotalCount(kHistogramName, 2);

  // The count shouldn't change when we're no longer in a session restore.
  FinishSessionRestore();
  SetBackgroundTabLoadingState(TAB_IS_NOT_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  FinishLoadingForegroundTab();
  histograms.ExpectTotalCount(kHistogramName, 2);
}

}  // namespace resource_coordinator
