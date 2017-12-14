// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_router_mojo_metrics.h"

#include "base/test/histogram_tester.h"
#include "base/version.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Bucket;
using testing::ElementsAre;

namespace media_router {

TEST(MediaRouterMojoMetricsTest, TestGetMediaRouteProviderVersion) {
  const base::Version kBrowserVersion("50.0.2396.71");
  EXPECT_EQ(MediaRouteProviderVersion::SAME_VERSION_AS_CHROME,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("50.0.2396.71"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::SAME_VERSION_AS_CHROME,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("50.0.2100.0"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::SAME_VERSION_AS_CHROME,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("51.0.2117.0"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::ONE_VERSION_BEHIND_CHROME,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("49.0.2138.0"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::MULTIPLE_VERSIONS_BEHIND_CHROME,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("47.0.1134.0"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::UNKNOWN,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("blargh"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::UNKNOWN,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version(""), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::UNKNOWN,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("-1.0.0.0"), kBrowserVersion));
  EXPECT_EQ(MediaRouteProviderVersion::UNKNOWN,
            MediaRouterMojoMetrics::GetMediaRouteProviderVersion(
                base::Version("0"), kBrowserVersion));
}

TEST(MediaRouterMojoMetricsTest, RecordCreateRouteResultCode) {
  const char(&kExtensionHistogram)[] =
      MediaRouterMojoMetrics::kHistogramProviderCreateRouteResult;
  const char(&kWiredDisplayHistogram)[] =
      MediaRouterMojoMetrics::kHistogramProviderCreateRouteResultWiredDisplay;
  base::HistogramTester tester;
  tester.ExpectTotalCount(kExtensionHistogram, 0);
  tester.ExpectTotalCount(kWiredDisplayHistogram, 0);

  MediaRouterMojoMetrics::RecordCreateRouteResultCode(
      MediaRouteProviderId::WIRED_DISPLAY, RouteRequestResult::OK);
  MediaRouterMojoMetrics::RecordCreateRouteResultCode(
      MediaRouteProviderId::EXTENSION, RouteRequestResult::SINK_NOT_FOUND);
  MediaRouterMojoMetrics::RecordCreateRouteResultCode(
      MediaRouteProviderId::WIRED_DISPLAY, RouteRequestResult::ROUTE_NOT_FOUND);
  MediaRouterMojoMetrics::RecordCreateRouteResultCode(
      MediaRouteProviderId::EXTENSION, RouteRequestResult::OK);
  MediaRouterMojoMetrics::RecordCreateRouteResultCode(
      MediaRouteProviderId::EXTENSION, RouteRequestResult::SINK_NOT_FOUND);

  tester.ExpectTotalCount(kExtensionHistogram, 3);
  EXPECT_THAT(
      tester.GetAllSamples(kExtensionHistogram),
      ElementsAre(
          Bucket(static_cast<int>(RouteRequestResult::OK), 1),
          Bucket(static_cast<int>(RouteRequestResult::SINK_NOT_FOUND), 2)));

  tester.ExpectTotalCount(kWiredDisplayHistogram, 2);
  EXPECT_THAT(
      tester.GetAllSamples(kWiredDisplayHistogram),
      ElementsAre(
          Bucket(static_cast<int>(RouteRequestResult::OK), 1),
          Bucket(static_cast<int>(RouteRequestResult::ROUTE_NOT_FOUND), 1)));
}

}  // namespace media_router
