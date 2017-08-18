// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "base/time/time.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/test/test_web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

using WebContents = content::WebContents;

namespace resource_coordinator {

class TabManagerStatsCollectorTest
    : public ChromeRenderViewHostTestHarness,
      public testing::WithParamInterface<
          std::tuple<bool,   // should_test_session_restore
                     bool>>  // should_test_background_tab_opening
{
 protected:
  TabManagerStatsCollectorTest() = default;
  ~TabManagerStatsCollectorTest() override = default;

  WebContents* CreateWebContents() {
    return content::TestWebContents::Create(profile(), nullptr);
  }

  TabManagerStatsCollector* GetTabManagerStatsCollector() {
    return tab_manager_.tab_manager_stats_collector_.get();
  }

  void StartSessionRestore() {
    GetTabManagerStatsCollector()->OnSessionRestoreStartedLoadingTabs();
  }

  void FinishSessionRestore() {
    GetTabManagerStatsCollector()->OnSessionRestoreFinishedLoadingTabs();
  }

  void StartBackgroundTabOpeningSession() {
    GetTabManagerStatsCollector()->OnBackgroundTabOpeningSessionStarted();
  }

  void FinishBackgroundTabOpeningSession() {
    GetTabManagerStatsCollector()->OnBackgroundTabOpeningSessionEnded();
  }

  TabManager::WebContentsData* GetWebContentsData(WebContents* contents) const {
    return tab_manager_.GetWebContentsData(contents);
  }

  void SetUp() override {
    std::tie(should_test_session_restore_,
             should_test_background_tab_opening_) = GetParam();
    ChromeRenderViewHostTestHarness::SetUp();
  }

 protected:
  TabManager tab_manager_;
  bool should_test_session_restore_;
  bool should_test_background_tab_opening_;
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

TEST_P(TabManagerStatsCollectorTabSwitchTest, HistogramsSwitchToTab) {
  const char kSessionRestoreHistogramName[] =
      "TabManager.SessionRestore.SwitchToTab";
  const char kBackgroundTabOpeningHistogramName[] =
      "TabManager.BackgroundTabOpening.SwitchToTab";

  struct HistogramParameters {
    const char* histogram_name;
    bool enabled;
  };
  std::vector<HistogramParameters> histogram_parameters = {
      HistogramParameters{kSessionRestoreHistogramName,
                          should_test_session_restore_},
      HistogramParameters{kBackgroundTabOpeningHistogramName,
                          should_test_background_tab_opening_},
  };

  base::HistogramTester histograms;
  histograms.ExpectTotalCount(kSessionRestoreHistogramName, 0);
  histograms.ExpectTotalCount(kBackgroundTabOpeningHistogramName, 0);

  std::unique_ptr<WebContents> tab1(CreateWebContents());
  std::unique_ptr<WebContents> tab2(CreateWebContents());
  SetForegroundAndBackgroundTabs(tab1.get(), tab2.get());

  if (should_test_session_restore_)
    StartSessionRestore();
  if (should_test_background_tab_opening_)
    StartBackgroundTabOpeningSession();

  SetBackgroundTabLoadingState(TAB_IS_NOT_LOADING);
  SetForegroundTabLoadingState(TAB_IS_NOT_LOADING);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  for (const auto& param : histogram_parameters) {
    if (param.enabled) {
      histograms.ExpectTotalCount(param.histogram_name, 2);
      histograms.ExpectBucketCount(param.histogram_name, TAB_IS_NOT_LOADING, 2);
    } else {
      histograms.ExpectTotalCount(param.histogram_name, 0);
    }
  }

  SetBackgroundTabLoadingState(TAB_IS_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADING);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  for (const auto& param : histogram_parameters) {
    if (param.enabled) {
      histograms.ExpectTotalCount(param.histogram_name, 5);
      histograms.ExpectBucketCount(param.histogram_name, TAB_IS_NOT_LOADING, 2);
      histograms.ExpectBucketCount(param.histogram_name, TAB_IS_LOADING, 3);
    } else {
      histograms.ExpectTotalCount(param.histogram_name, 0);
    }
  }

  SetBackgroundTabLoadingState(TAB_IS_LOADED);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  SwitchToBackgroundTab();
  for (const auto& param : histogram_parameters) {
    if (param.enabled) {
      histograms.ExpectTotalCount(param.histogram_name, 9);
      histograms.ExpectBucketCount(param.histogram_name, TAB_IS_NOT_LOADING, 2);
      histograms.ExpectBucketCount(param.histogram_name, TAB_IS_LOADING, 3);
      histograms.ExpectBucketCount(param.histogram_name, TAB_IS_LOADED, 4);
    } else {
      histograms.ExpectTotalCount(param.histogram_name, 0);
    }
  }
}

TEST_P(TabManagerStatsCollectorTabSwitchTest, HistogramsTabSwitchLoadTime) {
  const char kSessionRestoreHistogramName[] =
      "TabManager.Experimental.SessionRestore.TabSwitchLoadTime";
  const char kBackgroundTabOpeningHistogramName[] =
      "TabManager.Experimental.BackgroundTabOpening.TabSwitchLoadTime";

  base::HistogramTester histograms;
  histograms.ExpectTotalCount(kSessionRestoreHistogramName, 0);
  histograms.ExpectTotalCount(kBackgroundTabOpeningHistogramName, 0);

  std::unique_ptr<WebContents> tab1(CreateWebContents());
  std::unique_ptr<WebContents> tab2(CreateWebContents());
  SetForegroundAndBackgroundTabs(tab1.get(), tab2.get());

  if (should_test_session_restore_)
    StartSessionRestore();
  if (should_test_background_tab_opening_)
    StartBackgroundTabOpeningSession();

  SetBackgroundTabLoadingState(TAB_IS_NOT_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  FinishLoadingForegroundTab();
  histograms.ExpectTotalCount(kSessionRestoreHistogramName,
                              should_test_session_restore_ ? 1 : 0);
  histograms.ExpectTotalCount(kBackgroundTabOpeningHistogramName,
                              should_test_background_tab_opening_ ? 1 : 0);

  SetBackgroundTabLoadingState(TAB_IS_LOADING);
  SwitchToBackgroundTab();
  FinishLoadingForegroundTab();
  histograms.ExpectTotalCount(kSessionRestoreHistogramName,
                              should_test_session_restore_ ? 2 : 0);
  histograms.ExpectTotalCount(kBackgroundTabOpeningHistogramName,
                              should_test_background_tab_opening_ ? 2 : 0);

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
  histograms.ExpectTotalCount(kSessionRestoreHistogramName,
                              should_test_session_restore_ ? 2 : 0);
  histograms.ExpectTotalCount(kBackgroundTabOpeningHistogramName,
                              should_test_background_tab_opening_ ? 2 : 0);

  // The count shouldn't change when we're no longer in a session restore or
  // background tab opening.
  if (should_test_session_restore_)
    FinishSessionRestore();
  if (should_test_background_tab_opening_)
    FinishBackgroundTabOpeningSession();
  SetBackgroundTabLoadingState(TAB_IS_NOT_LOADING);
  SetForegroundTabLoadingState(TAB_IS_LOADED);
  SwitchToBackgroundTab();
  FinishLoadingForegroundTab();
  histograms.ExpectTotalCount(kSessionRestoreHistogramName,
                              should_test_session_restore_ ? 2 : 0);
  histograms.ExpectTotalCount(kBackgroundTabOpeningHistogramName,
                              should_test_background_tab_opening_ ? 2 : 0);
}

TEST_P(TabManagerStatsCollectorTest, HistogramsExpectedTaskQueueingDuration) {
  auto* stats_collector = GetTabManagerStatsCollector();

  base::HistogramTester histograms;
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration,
      0);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration,
      0);

  const base::TimeDelta kEQT = base::TimeDelta::FromMilliseconds(1);
  web_contents()->WasShown();

  // No metrics recorded because there is no session restore or background tab
  // opening.
  stats_collector->RecordExpectedTaskQueueingDuration(web_contents(), kEQT);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration,
      0);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration,
      0);

  if (should_test_session_restore_)
    StartSessionRestore();
  if (should_test_background_tab_opening_)
    StartBackgroundTabOpeningSession();

  // No metrics recorded because the tab is background.
  web_contents()->WasHidden();
  stats_collector->RecordExpectedTaskQueueingDuration(web_contents(), kEQT);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration,
      0);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration,
      0);

  web_contents()->WasShown();
  stats_collector->RecordExpectedTaskQueueingDuration(web_contents(), kEQT);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration,
      should_test_session_restore_ ? 1 : 0);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration,
      should_test_background_tab_opening_ ? 1 : 0);

  if (should_test_session_restore_)
    FinishSessionRestore();
  if (should_test_background_tab_opening_)
    FinishBackgroundTabOpeningSession();

  // No metrics recorded because there is no session restore or background tab
  // opening.
  stats_collector->RecordExpectedTaskQueueingDuration(web_contents(), kEQT);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramSessionRestoreForegroundTabExpectedTaskQueueingDuration,
      should_test_session_restore_ ? 1 : 0);
  histograms.ExpectTotalCount(
      TabManagerStatsCollector::
          kHistogramBackgroundTabOpeningForegroundTabExpectedTaskQueueingDuration,
      should_test_background_tab_opening_ ? 1 : 0);
}

INSTANTIATE_TEST_CASE_P(
    ,
    TabManagerStatsCollectorTabSwitchTest,
    ::testing::Values(std::make_tuple(false,   // Session restore
                                      false),  // Background tab opening
                      std::make_tuple(true, false),
                      std::make_tuple(false, true),
                      std::make_tuple(true, true)));
INSTANTIATE_TEST_CASE_P(
    ,
    TabManagerStatsCollectorTest,
    ::testing::Values(std::make_tuple(false,   // Session restore
                                      false),  // Background tab opening
                      std::make_tuple(true, false),
                      std::make_tuple(false, true),
                      std::make_tuple(true, true)));

}  // namespace resource_coordinator
