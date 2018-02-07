// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/thread_profiler.h"

#include "base/macros.h"
#include "base/test/bind_test_util.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ThreadProfilerTest, PeriodicSamplingScheduler) {
  // Initialized to the "zero" time.
  const base::TimeTicks start_time;
  const base::TimeTicks artificial_now =
      start_time - base::TimeDelta::FromSeconds(1000);
  double rand_double = 0.0;
  const base::TimeDelta sampling_duration = base::TimeDelta::FromSeconds(30);
  const double fraction_of_execution_time_to_sample = 0.01;

  const base::TimeDelta expected_period =
      sampling_duration / fraction_of_execution_time_to_sample;

  PeriodicSamplingScheduler::Dependencies dependencies(
      base::BindLambdaForTesting([&rand_double]() { return rand_double; }),
      base::BindLambdaForTesting(
          [artificial_now]() { return artificial_now; }));

  PeriodicSamplingScheduler scheduler(sampling_duration,
                                      fraction_of_execution_time_to_sample,
                                      start_time, dependencies);
  // The first collection should be exactly at the start time, since the random
  // value is 0.0.
  EXPECT_EQ(base::TimeDelta::FromSeconds(1000),
            scheduler.GetTimeToNextCollection());

  // With a random value of 1.0 the second collection should be at the end of
  // the second period.
  rand_double = 1.0;
  EXPECT_EQ(base::TimeDelta::FromSeconds(1000) + 2 * expected_period -
                sampling_duration,
            scheduler.GetTimeToNextCollection());

  // With a random value of 0.25 the second collection should be a quarter into
  // the third period exclusive of the sampling duration.
  rand_double = 0.25;
  EXPECT_EQ(base::TimeDelta::FromSeconds(1000) + 2 * expected_period +
                0.25 * (expected_period - sampling_duration),
            scheduler.GetTimeToNextCollection());
}
