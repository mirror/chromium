// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/fps_meter.h"

#include "base/macros.h"
#include "chrome/browser/vr/test/animation_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

static constexpr double kTolerance = 0.01;

}  // namespace

TEST(FPSMeter, GetFPSWithTooFewFrames) {
  FPSMeter meter;
  EXPECT_FALSE(meter.CanComputeFPS());
  EXPECT_FLOAT_EQ(0.0, meter.GetFPS());

  meter.AddFrame(MicrosecondsToTicks(16000));
  EXPECT_FALSE(meter.CanComputeFPS());
  EXPECT_FLOAT_EQ(0.0, meter.GetFPS());

  meter.AddFrame(MicrosecondsToTicks(32000));
  EXPECT_TRUE(meter.CanComputeFPS());
  EXPECT_LT(0.0, meter.GetFPS());
}

TEST(FPSMeter, AccurateFPSWithManyFrames) {
  FPSMeter meter;
  EXPECT_FALSE(meter.CanComputeFPS());
  EXPECT_FLOAT_EQ(0.0, meter.GetFPS());

  base::TimeTicks now = MicrosecondsToTicks(1);
  base::TimeDelta frame_time = MicrosecondsToDelta(16666);

  meter.AddFrame(now);
  EXPECT_FALSE(meter.CanComputeFPS());
  EXPECT_FLOAT_EQ(0.0, meter.GetFPS());

  for (size_t i = 0; i < 2 * meter.GetNumFrameTimes(); ++i) {
    now += frame_time;
    meter.AddFrame(now);
    EXPECT_TRUE(meter.CanComputeFPS());
    EXPECT_NEAR(60.0, meter.GetFPS(), kTolerance);
  }
}

TEST(FPSMeter, AccurateFPSWithHigherFramerate) {
  FPSMeter meter;
  EXPECT_FALSE(meter.CanComputeFPS());
  EXPECT_FLOAT_EQ(0.0, meter.GetFPS());

  base::TimeTicks now = MicrosecondsToTicks(1);
  base::TimeDelta frame_time = base::TimeDelta::FromSecondsD(1.0 / 90.0);

  meter.AddFrame(now);
  EXPECT_FALSE(meter.CanComputeFPS());
  EXPECT_FLOAT_EQ(0.0, meter.GetFPS());

  for (int i = 0; i < 5; ++i) {
    now += frame_time;
    meter.AddFrame(now);
    EXPECT_TRUE(meter.CanComputeFPS());
    EXPECT_NEAR(1.0 / frame_time.InSecondsF(), meter.GetFPS(), kTolerance);
  }
}

TEST(SlidingAverage, Basics) {
  SlidingAverage meter(5);

  // No values yet
  EXPECT_EQ(42, meter.GetAverageOrDefault(42));
  EXPECT_EQ(0, meter.GetAverage());

  meter.AddSample(100);
  EXPECT_EQ(100, meter.GetAverageOrDefault(42));
  EXPECT_EQ(100, meter.GetAverage());

  meter.AddSample(200);
  EXPECT_EQ(150, meter.GetAverage());

  meter.AddSample(10);
  EXPECT_EQ(103, meter.GetAverage());

  meter.AddSample(10);
  EXPECT_EQ(80, meter.GetAverage());

  meter.AddSample(10);
  EXPECT_EQ(66, meter.GetAverage());

  meter.AddSample(10);
  EXPECT_EQ(48, meter.GetAverage());

  meter.AddSample(10);
  EXPECT_EQ(10, meter.GetAverage());

  meter.AddSample(110);
  EXPECT_EQ(30, meter.GetAverage());
}

TEST(SlidingTimeDeltaAverage, Basics) {
  SlidingTimeDeltaAverage meter(5);

  EXPECT_EQ(base::TimeDelta::FromSeconds(42),
            meter.GetAverageOrDefault(base::TimeDelta::FromSeconds(42)));
  EXPECT_EQ(base::TimeDelta(), meter.GetAverage());

  meter.AddSample(base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(base::TimeDelta::FromSeconds(100),
            meter.GetAverageOrDefault(base::TimeDelta::FromSeconds(42)));
  EXPECT_EQ(base::TimeDelta::FromSeconds(100), meter.GetAverage());

  meter.AddSample(base::TimeDelta::FromMilliseconds(200000));
  EXPECT_EQ(base::TimeDelta::FromSeconds(150), meter.GetAverage());
}

TEST(HeuristicVSyncAverage, Basics) {
  HeuristicVSyncAverage meter(5);

  // Synthesize some samples based on a 20ms interval (50fps),
  // starting out slow and settling on the true value with
  // a fair amount of skipped frames.
  meter.AddSample(base::TimeDelta::FromMilliseconds(-1466655));
  meter.AddSample(base::TimeDelta::FromMilliseconds(20));
  meter.AddSample(base::TimeDelta::FromMilliseconds(40));
  meter.AddSample(base::TimeDelta::FromMilliseconds(60));
  meter.AddSample(base::TimeDelta::FromMilliseconds(40));
  meter.AddSample(base::TimeDelta::FromMilliseconds(21));
  meter.AddSample(base::TimeDelta::FromMilliseconds(19));
  meter.AddSample(base::TimeDelta::FromMilliseconds(20));
  meter.AddSample(base::TimeDelta::FromMilliseconds(41));
  meter.AddSample(base::TimeDelta::FromMilliseconds(20));
  meter.AddSample(base::TimeDelta::FromMilliseconds(22));
  meter.AddSample(base::TimeDelta::FromMilliseconds(20));
  meter.AddSample(base::TimeDelta::FromMilliseconds(22));
  meter.AddSample(base::TimeDelta::FromMilliseconds(41));
  meter.AddSample(base::TimeDelta::FromMilliseconds(22));
  meter.AddSample(base::TimeDelta::FromMilliseconds(39));

  EXPECT_EQ(20, meter.GetAverage().InMilliseconds());

  // Add a bunch of slow frames, this shouldn't confuse it.
  meter.AddSample(base::TimeDelta::FromMilliseconds(41));
  meter.AddSample(base::TimeDelta::FromMilliseconds(39));
  meter.AddSample(base::TimeDelta::FromMilliseconds(61));
  meter.AddSample(base::TimeDelta::FromMilliseconds(99));
  meter.AddSample(base::TimeDelta::FromMilliseconds(42));
  meter.AddSample(base::TimeDelta::FromMilliseconds(81));

  EXPECT_EQ(20, meter.GetAverage().InMilliseconds());
}

}  // namespace vr
