// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_RETRY_STRATEGY_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_RETRY_STRATEGY_H_

#include <algorithm>
#include <cmath>
#include <memory>

#include "base/logging.h"
#include "base/macros.h"

namespace media_router {

class RetryStrategy;

// A strategy for retry.
class RetryStrategy {
 public:
  // Returns true if should conduct more retries given current number of retry
  // attempts.
  // |num_attempts|: number of retry attempts already conducted.
  virtual bool TryAgain(int num_attempts) = 0;

  // Returns the delay number in seconds given current number of retry attempts.
  // |num_attempts|: number of retry attempts already conducted.
  virtual int GetDelayInSeconds(int num_attempts) = 0;

  // Returns a RetryStrategy object that allows |max_attempts| number of retries
  // and wait for |delay_in_seconds| between two retry attempts.
  static std::unique_ptr<RetryStrategy> UniformDelay(int max_attempts,
                                                     int delay_in_seconds);
};

// Exponential backoff retry strategy.
class ExponentialBackoff : public RetryStrategy {
 public:
  ExponentialBackoff(int max_attempts,
                     int first_delay_in_seconds,
                     double multiplier);

  // RetryStrategy implementation.
  bool TryAgain(int num_attempts) override;
  int GetDelayInSeconds(int num_attempts) override;

 private:
  // Max number of retry attempts allowed.
  int max_attempts_;
  // Time to wait before invoking first retry attempt.
  int first_delay_in_seconds_;
  // Parameter used to calculate delay numbers in seconds. If set to 1.0,
  // adopt uniform delay between two retry attempts.
  double multiplier_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_RETRY_STRATEGY_H_
