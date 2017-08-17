// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/retry_strategy.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

TEST(RetryStrategyExponentialBackoffTest, TestTryAgain) {
  // No retry.
  int max_attempt = 0;
  int delay_in_seconds = 5;
  double multiplier = 2.0;
  ExponentialBackoff retry_strategy(max_attempt, delay_in_seconds, multiplier);
  EXPECT_FALSE(retry_strategy.TryAgain(max_attempt));

  // Three retries.
  max_attempt = 3;
  retry_strategy =
      ExponentialBackoff(max_attempt, delay_in_seconds, multiplier);
  EXPECT_TRUE(retry_strategy.TryAgain(1));
  EXPECT_FALSE(retry_strategy.TryAgain(max_attempt));
}

TEST(RetryStrategyExponentialBackoffTest, TestGetDelayInSeconds) {
  // Uniform delay
  int max_attempt = 3;
  int delay_in_sec = 5;
  double multiplier = 1.0;
  ExponentialBackoff retry_strategy(max_attempt, delay_in_sec, multiplier);
  EXPECT_EQ(delay_in_sec, retry_strategy.GetDelayInSeconds(0));
  EXPECT_EQ(delay_in_sec, retry_strategy.GetDelayInSeconds(1));
  EXPECT_EQ(delay_in_sec, retry_strategy.GetDelayInSeconds(2));
  EXPECT_EQ(-1, retry_strategy.GetDelayInSeconds(3));

  // Exponential backoff
  max_attempt = 5;
  delay_in_sec = 30;
  multiplier = 2.0;
  retry_strategy = ExponentialBackoff(max_attempt, delay_in_sec, multiplier);
  EXPECT_EQ(delay_in_sec, retry_strategy.GetDelayInSeconds(0));
  EXPECT_EQ(delay_in_sec * 2, retry_strategy.GetDelayInSeconds(1));
  EXPECT_EQ(delay_in_sec * 2 * 2, retry_strategy.GetDelayInSeconds(2));
  EXPECT_EQ(-1, retry_strategy.GetDelayInSeconds(max_attempt));
}

}  // namespace media_router
