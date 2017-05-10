// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/system_time_provider.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement_tracker {

namespace {

class SystemTimeProviderTest : public ::testing::Test {
 public:
  SystemTimeProviderTest() = default;

 protected:
  SystemTimeProvider time_provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemTimeProviderTest);
};

}  // namespace

TEST_F(SystemTimeProviderTest, TestGetCurrentDay) {
  // Calculate the expected number of days since epoch.
  base::Time now = base::Time::Now();
  base::Time epoch = base::Time::UnixEpoch();
  base::TimeDelta delta = now - epoch;
  // This will fail if the delta is not within the limits of uint32_t.
  uint32_t expected_delta_days = base::checked_cast<uint32_t>(delta.InDays());

  // The expected number of days should match what the Model yields.
  uint32_t days_since_epoch = time_provider_.GetCurrentDay();
  EXPECT_EQ(expected_delta_days, days_since_epoch);
}

}  // namespace feature_engagement_tracker
