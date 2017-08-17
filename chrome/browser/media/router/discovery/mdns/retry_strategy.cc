// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/retry_strategy.h"

#include "base/memory/ptr_util.h"

namespace media_router {

// static
std::unique_ptr<RetryStrategy> RetryStrategy::UniformDelay(
    int max_attempts,
    int delay_in_seconds) {
  return base::MakeUnique<ExponentialBackoff>(max_attempts, delay_in_seconds,
                                              1.0);
}

ExponentialBackoff::ExponentialBackoff(int max_attempts,
                                       int first_delay_in_seconds,
                                       double multiplier)
    : max_attempts_(max_attempts),
      first_delay_in_seconds_(first_delay_in_seconds),
      multiplier_(multiplier) {}

bool ExponentialBackoff::TryAgain(int num_attempts) {
  return num_attempts < max_attempts_;
}

int ExponentialBackoff::GetDelayInSeconds(int num_attempts) {
  if (!TryAgain(num_attempts))
    return -1;

  int delay_in_seconds =
      first_delay_in_seconds_ * std::pow(multiplier_, num_attempts);
  return delay_in_seconds;
}

}  // namespace media_router
