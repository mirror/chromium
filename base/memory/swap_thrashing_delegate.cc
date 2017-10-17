// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Dummy implementation of the SwapThrashingDelegate, the platforms that need
// it should implement these functions in a platform specific file.

#include "base/memory/swap_thrashing_monitor.h"

#include "base/logging.h"

namespace base {

// These values should be defined per platform.
const int SwapThrashingDelegate::kPollingIntervalMs = 0;
const int SwapThrashingDelegate::kNoneToSuspectedIntervalMs = 0;
const int SwapThrashingDelegate::kSuspectedIntervalCoolDownMs = 0;
const int SwapThrashingDelegate::kSuspectedToConfirmedIntervalMs = 0;
const int SwapThrashingDelegate::kConfirmedCoolDownMs = 0;

SwapThrashingDelegate::SwapThrashingDelegate() {}

SwapThrashingDelegate::~SwapThrashingDelegate() {}

int SwapThrashingDelegate::GetPollingIntervalMs() {
  return kPollingIntervalMs;
}

SwapThrashingDelegate::SwapThrashingLevel
SwapThrashingDelegate::CalculateCurrentSwapThrashingLevel(
    SwapThrashingDelegate::SwapThrashingLevel previous_state) {
  NOTIMPLEMENTED();
  return previous_state;
}

}  // namespace base
