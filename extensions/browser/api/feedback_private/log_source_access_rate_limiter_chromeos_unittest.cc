// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/feedback_private/log_source_access_rate_limiter.h"

#include "base/test/simple_test_tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

using TimeDelta = base::TimeDelta;

}  // namespace

class LogSourceAccessRateLimiterTest : public ::testing::Test {
 public:
  LogSourceAccessRateLimiterTest()
      : test_clock_(new base::SimpleTestTickClock) {
    // |test_clock_| must start out at something other than 0, which is
    // interpreted as an invalid value.
    test_clock_->Advance(TimeDelta::FromMilliseconds(100));
  }

  ~LogSourceAccessRateLimiterTest() override {}

 protected:
  std::unique_ptr<base::SimpleTestTickClock> test_clock_;

  std::unique_ptr<LogSourceAccessRateLimiter> limiter_;
};

TEST_F(LogSourceAccessRateLimiterTest, MaxAccessCountOfZero) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(0, TimeDelta::FromMilliseconds(100)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
}

TEST_F(LogSourceAccessRateLimiterTest, NormalRepeatedAccess) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(5, TimeDelta::FromMilliseconds(100)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
}

TEST_F(LogSourceAccessRateLimiterTest, RechargeWhenDry) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(5, TimeDelta::FromMilliseconds(100)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  test_clock_->Advance(TimeDelta::FromMilliseconds(100));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  test_clock_->Advance(TimeDelta::FromMilliseconds(500));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
}

TEST_F(LogSourceAccessRateLimiterTest, RechargeTimeOfZero) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(5, TimeDelta::FromMilliseconds(0)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  // Unlimited number of accesses.
  for (int i = 0; i < 100; ++i)
    EXPECT_TRUE(limiter_->TryAccess()) << i;

  // Advancing should not make a difference.
  test_clock_->Advance(TimeDelta::FromMilliseconds(100));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());

  test_clock_->Advance(TimeDelta::FromMilliseconds(500));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
}

TEST_F(LogSourceAccessRateLimiterTest, RechargeToMax) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(5, TimeDelta::FromMilliseconds(100)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  // Should not exceed the max number of accesses.
  test_clock_->Advance(TimeDelta::FromMilliseconds(1000));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
}

TEST_F(LogSourceAccessRateLimiterTest, IncrementalRecharge) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(5, TimeDelta::FromMilliseconds(100)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  // Has not yet hit the full recharge period.
  test_clock_->Advance(TimeDelta::FromMilliseconds(50));
  EXPECT_FALSE(limiter_->TryAccess());

  // Has finally hit the full recharge period.
  test_clock_->Advance(TimeDelta::FromMilliseconds(50));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  // This only recharges two full periods.
  test_clock_->Advance(TimeDelta::FromMilliseconds(250));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  // This finishes recharging three full periods.
  test_clock_->Advance(TimeDelta::FromMilliseconds(250));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
}

TEST_F(LogSourceAccessRateLimiterTest, IncrementalRechargeToMax) {
  limiter_.reset(
      new LogSourceAccessRateLimiter(5, TimeDelta::FromMilliseconds(100)));
  limiter_->SetTickClockForTesting(test_clock_.get());

  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());

  // This only recharges two full periods.
  test_clock_->Advance(TimeDelta::FromMilliseconds(250));
  // This finishes recharging three full periods, but will not recharge over the
  // additional periods.
  test_clock_->Advance(TimeDelta::FromMilliseconds(450));
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_TRUE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
  EXPECT_FALSE(limiter_->TryAccess());
}

}  // namespace extensions
