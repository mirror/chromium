// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl_params.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

TEST(CastMediaSinkServiceImplParamsTest,
     TestInitRetryParametersWithFeatureDisabled) {
  // Feature not enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEnableCastDiscovery);

  EXPECT_EQ(CastChannelRetryParams(),
            CastChannelRetryParams::GetFromFieldTrialParam());
  EXPECT_EQ(CastChannelOpenParams(),
            CastChannelOpenParams::GetFromFieldTrialParam());
}

TEST(CastMediaSinkServiceImplParamsTest, TestInitParameters) {
  base::test::ScopedFeatureList scoped_feature_list;
  std::map<std::string, std::string> params;
  params[CastChannelRetryParams::kParamNameInitialDelayInMilliSeconds] = "2000";
  params[CastChannelRetryParams::kParamNameMaxRetryAttempts] = "20";
  params[CastChannelRetryParams::kParamNameExponential] = "2.0";

  params[CastChannelOpenParams::kParamNameConnectTimeoutInSeconds] = "20";
  params[CastChannelOpenParams::kParamNamePingIntervalInSeconds] = "15";
  params[CastChannelOpenParams::kParamNameLivenessTimeoutInSeconds] = "30";
  params[CastChannelOpenParams::kParamNameDynamicTimeoutDeltaInSeconds] = "7";
  scoped_feature_list.InitAndEnableFeatureWithParameters(kEnableCastDiscovery,
                                                         params);

  CastChannelRetryParams expected_retry_params;
  expected_retry_params.initial_delay_in_milliseconds = 2000;
  expected_retry_params.max_retry_attempts = 20;
  expected_retry_params.multiply_factor = 2.0;
  EXPECT_EQ(expected_retry_params,
            CastChannelRetryParams::GetFromFieldTrialParam());

  CastChannelOpenParams expected_open_params;
  expected_open_params.connect_timeout_in_seconds = 20;
  expected_open_params.ping_interval_in_seconds = 15;
  expected_open_params.liveness_timeout_in_seconds = 30;
  expected_open_params.dynamic_timeout_delta_in_seconds = 7;
  EXPECT_EQ(expected_open_params,
            CastChannelOpenParams::GetFromFieldTrialParam());
}

TEST(CastMediaSinkServiceImplParamsTest,
     TestInitRetryParametersWithDefaultValue) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kEnableCastDiscovery);

  EXPECT_EQ(CastChannelRetryParams(),
            CastChannelRetryParams::GetFromFieldTrialParam());
  EXPECT_EQ(CastChannelOpenParams(),
            CastChannelOpenParams::GetFromFieldTrialParam());
}

}  // namespace media_router
