// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/Histogram.h"

#include "base/metrics/histogram_samples.h"
#include "platform/wtf/Time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class TestCustomCountHistogram : public CustomCountHistogram {
 public:
  TestCustomCountHistogram(const char* name,
                           base::HistogramBase::Sample min,
                           base::HistogramBase::Sample max,
                           int32_t bucket_count)
      : CustomCountHistogram(name, min, max, bucket_count) {}

  base::HistogramBase* Histogram() { return histogram_; }
};

class ScopedUsHistogramTimerTest : public ::testing::Test {
 protected:
  static double MockedTime() { return mocked_time_; }

  static void SetMockedTime(double mocked_time) { mocked_time_ = mocked_time; }

  static double mocked_time_;
};

double ScopedUsHistogramTimerTest::mocked_time_ = 0.0;

TEST_F(ScopedUsHistogramTimerTest, Basic) {
  SetTimeFunctionsForTesting(&MockedTime);
  TestCustomCountHistogram scoped_us_counter("test", 0, 10000000, 50);
  {
    EXPECT_EQ(0.0, MockedTime());
    ScopedUsHistogramTimer timer(scoped_us_counter);
    SetMockedTime(0.5);
  }
  // 500ms == 500000us
  EXPECT_EQ(500000, scoped_us_counter.Histogram()->SnapshotSamples()->sum());
}

}  // namespace blink
