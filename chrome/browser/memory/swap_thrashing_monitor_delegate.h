// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_H_

#include "build/build_config.h"
#include "chrome/browser/memory/swap_thrashing_level.h"

#if defined(OS_WIN)
#include "chrome/browser/memory/swap_thrashing_monitor_delegate_win.h"
#endif

namespace memory {

#if defined(OS_WIN)
using SwapThrashingMonitorDelegate = SwapThrashingMonitorDelegateWin;
#else
// Dummy definition of a SwapThrashingMonitorDelegate, the platforms interested
// in monitoring the swap thrashing state should implement this class.
class SwapThrashingMonitorDelegate {
 public:
  // Constructor/Destructor.
  SwapThrashingMonitorDelegate() {}
  virtual ~SwapThrashingMonitorDelegate() {}

  // Dummy implementation.
  SwapThrashingLevel SampleAndCalculateSwapThrashingLevel() {
    return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegate);
};
#endif

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_H_
