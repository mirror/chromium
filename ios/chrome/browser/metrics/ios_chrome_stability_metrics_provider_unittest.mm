// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/metrics/ios_chrome_stability_metrics_provider.h"

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#import "ios/web/public/web_state/navigation_context.h"
#import "ios/web/web_state/navigation_context_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/metrics_proto/system_profile.pb.h"

namespace {

static web::WebState* kNullWebState = nullptr;
static const bool kShouldNotLogPageLoad = false;

class IOSChromeStabilityMetricsProviderTest : public PlatformTest {
 protected:
  IOSChromeStabilityMetricsProviderTest()
      : prefs_(new TestingPrefServiceSimple) {
    metrics::StabilityMetricsHelper::RegisterPrefs(prefs()->registry());
  }

  TestingPrefServiceSimple* prefs() { return prefs_.get(); }

  void VerifyNavigationStat(
      web::NavigationContext* context,
      IOSChromeStabilityMetricsProvider::StabilityMetricEventType
          expected_event_type,
      bool should_log_page_load) {
    IOSChromeStabilityMetricsProvider provider(prefs());
    provider.OnRecordingEnabled();
    provider.WebStateDidStartNavigation(kNullWebState, context);

    histogram_tester_.ExpectUniqueSample(
        IOSChromeStabilityMetricsProvider::kPageLoadCountMigrationEventKey,
        expected_event_type, 1);

    metrics::SystemProfileProto system_profile;
    provider.ProvideStabilityMetrics(&system_profile);
    int expected_page_load_count = should_log_page_load ? 1 : 0;
    EXPECT_EQ(expected_page_load_count,
              system_profile.stability().page_load_count());
  }

 protected:
  base::HistogramTester histogram_tester_;

 private:
  std::unique_ptr<TestingPrefServiceSimple> prefs_;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeStabilityMetricsProviderTest);
};

}  // namespace

TEST_F(IOSChromeStabilityMetricsProviderTest, LogLoadStart) {
  IOSChromeStabilityMetricsProvider provider(prefs());

  // A load should not increment metrics if recording is disabled.
  provider.WebStateDidStartLoading(nullptr);

  metrics::SystemProfileProto system_profile;

  // Call ProvideStabilityMetrics to check that it will force pending tasks to
  // be executed immediately.
  provider.ProvideStabilityMetrics(&system_profile);

  EXPECT_EQ(0, system_profile.stability().page_load_count());
  EXPECT_TRUE(histogram_tester_
                  .GetTotalCountsForPrefix(IOSChromeStabilityMetricsProvider::
                                               kPageLoadCountMigrationEventKey)
                  .empty());

  // A load should increment metrics if recording is enabled.
  provider.OnRecordingEnabled();
  provider.WebStateDidStartLoading(nullptr);

  system_profile.Clear();
  provider.ProvideStabilityMetrics(&system_profile);

  EXPECT_EQ(1, system_profile.stability().page_load_count());
  histogram_tester_.ExpectUniqueSample(
      IOSChromeStabilityMetricsProvider::kPageLoadCountMigrationEventKey,
      IOSChromeStabilityMetricsProvider::LOADING_STARTED, 1);
}

TEST_F(IOSChromeStabilityMetricsProviderTest,
       SameDocumentNavigationShouldNotLogPageLoad) {
  std::unique_ptr<web::NavigationContextImpl> context =
      web::NavigationContextImpl::CreateNavigationContext(
          kNullWebState, GURL(), ui::PageTransition::PAGE_TRANSITION_TYPED,
          false /* is_renderer_initiated */);
  context->SetIsSameDocument(true);
  VerifyNavigationStat(
      context.get(),
      IOSChromeStabilityMetricsProvider::SAME_DOCUMENT_NAVIGATION,
      kShouldNotLogPageLoad);
}

TEST_F(IOSChromeStabilityMetricsProviderTest,
       ChromeUrlNavigationShouldNotLogPageLoad) {
  auto context = web::NavigationContextImpl::CreateNavigationContext(
      kNullWebState, GURL("chrome://newtab"),
      ui::PageTransition::PAGE_TRANSITION_TYPED,
      false /* is_renderer_initiated */);
  VerifyNavigationStat(context.get(),
                       IOSChromeStabilityMetricsProvider::CHROME_URL_NAVIGATION,
                       kShouldNotLogPageLoad);
}

TEST_F(IOSChromeStabilityMetricsProviderTest, WebNavigationShouldLogPageLoad) {
  auto context = web::NavigationContextImpl::CreateNavigationContext(
      kNullWebState, GURL(), ui::PAGE_TRANSITION_TYPED,
      false /* is_renderer_initiated */);
  VerifyNavigationStat(context.get(),
                       IOSChromeStabilityMetricsProvider::PAGE_LOAD_NAVIGATION,
                       // TODO(crbug.com/786547): change to kShouldLogPageLoad
                       // once page load count cuts over to be based on
                       // DidStartNavigation.
                       kShouldNotLogPageLoad);
}

TEST_F(IOSChromeStabilityMetricsProviderTest, LogRendererCrash) {
  IOSChromeStabilityMetricsProvider provider(prefs());

  // A crash should not increment the renderer crash count if recording is
  // disabled.
  provider.LogRendererCrash();

  metrics::SystemProfileProto system_profile;

  // Call ProvideStabilityMetrics to check that it will force pending tasks to
  // be executed immediately.
  provider.ProvideStabilityMetrics(&system_profile);

  EXPECT_EQ(0, system_profile.stability().renderer_crash_count());
  EXPECT_EQ(0, system_profile.stability().renderer_failed_launch_count());
  EXPECT_EQ(0, system_profile.stability().extension_renderer_crash_count());

  // A crash should increment the renderer crash count if recording is
  // enabled.
  provider.OnRecordingEnabled();
  provider.LogRendererCrash();

  system_profile.Clear();
  provider.ProvideStabilityMetrics(&system_profile);

  EXPECT_EQ(1, system_profile.stability().renderer_crash_count());
  EXPECT_EQ(0, system_profile.stability().renderer_failed_launch_count());
  EXPECT_EQ(0, system_profile.stability().extension_renderer_crash_count());
}
