// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/encoder_bitrate_filter.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

TEST(EncoderBitrateFilterTest, TargetBitrate_SameValues) {
  static constexpr int kBandwidthKbps = 1000000;
  EncoderBitrateFilter filter(1000);
  for (int i = 0; i < 1000; i++) {
    filter.SetBandwidthEstimateKbps(kBandwidthKbps);
    EXPECT_EQ(kBandwidthKbps, filter.GetTargetBitrateKbps());
  }
}

TEST(EncoderBitrateFilterTest, WontChangeBitrateForASharpIncrement) {
  static constexpr int kBandwidthKbps = 1000000;
  EncoderBitrateFilter filter(1000);
  for (int i = 0; i < 1000; i++) {
    if (i == 500) {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps * 5);
    } else {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps);
    }
    EXPECT_EQ(kBandwidthKbps, filter.GetTargetBitrateKbps());
  }
}

TEST(EncoderBitrateFilterTest, WontChangeBitrateForASharpDecrement) {
  static constexpr int kBandwidthKbps = 1000000;
  EncoderBitrateFilter filter(1000);
  for (int i = 0; i < 1000; i++) {
    if (i == 500) {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps / 5);
    } else {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps);
    }
    EXPECT_EQ(kBandwidthKbps, filter.GetTargetBitrateKbps());
  }
}

TEST(EncoderBitrateFilterTest, WontChangeBitrateIfRateIsNotMeet) {
  static constexpr int kBandwidthKbps = 1000000;
  EncoderBitrateFilter filter(1000);
  for (int i = 0; i < 1000; i++) {
    if (i < 500) {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps);
    } else {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps * 1.2);
    }
    EXPECT_EQ(kBandwidthKbps, filter.GetTargetBitrateKbps());
  }
}

TEST(EncoderBitrateFilterTest, ChangeBitrateIfRateIsMeet) {
  static constexpr int kBandwidthKbps = 1000000;
  EncoderBitrateFilter filter(1000);
  for (int i = 0; i < 1000; i++) {
    if (i < 500) {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps);
      EXPECT_EQ(kBandwidthKbps, filter.GetTargetBitrateKbps());
    } else {
      filter.SetBandwidthEstimateKbps(kBandwidthKbps * 2);
      // We do not want to test the implementation details. But the filter
      // should be able to take effect within 30 frames (1 second) if the
      // bandwidth is doubled.
      if (i > 530) {
        EXPECT_GT(filter.GetTargetBitrateKbps(), kBandwidthKbps);
      }
    }
  }
  // After 500 frames (~16 seconds), the weight of old bandwidth values should
  // be small enough to have very little impact to the result.
  EXPECT_GT(filter.GetTargetBitrateKbps(), kBandwidthKbps * 1.7);
}

TEST(EncoderBitrateFilterTest, ReturnDefaultValue) {
  EncoderBitrateFilter filter(1000);
  EXPECT_GT(filter.GetTargetBitrateKbps(), 0);
}

}  // namespace remoting
