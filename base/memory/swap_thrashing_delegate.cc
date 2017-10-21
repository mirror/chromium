// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Dummy implementation of the SwapThrashingMonitorDelegateInterface, the
// platforms that need it should implement these functions in a platform
// specific file.

#include "base/memory/swap_thrashing_monitor.h"

#include "base/logging.h"

namespace base {

SwapThrashingMonitorDelegateInterface::SwapThrashingMonitorDelegateInterface() {
}

SwapThrashingMonitorDelegateInterface::
    ~SwapThrashingMonitorDelegateInterface() {}

int SwapThrashingMonitorDelegateInterface::GetPollingIntervalMs() {
  return 0;
}

SwapThrashingMonitorDelegateInterface::SwapThrashingLevel
SwapThrashingMonitorDelegateInterface::CalculateCurrentSwapThrashingLevel(
    SwapThrashingMonitorDelegateInterface::SwapThrashingLevel previous_state) {
  return previous_state;
}

}  // namespace base
