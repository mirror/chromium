// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_SWAP_METRICS_DRIVER_MAC_H_
#define CONTENT_BROWSER_MEMORY_SWAP_METRICS_DRIVER_MAC_H_

#include "content/browser/memory/swap_metrics_driver.h"

#include <memory>

#include "base/mac/scoped_mach_port.h"
#include "base/time/time.h"

namespace content {

class SwapMetricsDriverMac : public SwapMetricsDriver {
 public:
  SwapMetricsDriverMac(std::unique_ptr<Delegate> delegate,
                       const base::TimeDelta update_interval);
  ~SwapMetricsDriverMac() override;

 protected:
  SwapMetricsUpdateResult UpdateMetricsInternal(
      base::TimeDelta interval) override;

 private:
  base::mac::ScopedMachSendRight host_;

  uint64_t last_swapins_ = 0;
  uint64_t last_swapouts_ = 0;
  uint64_t last_decompressions_ = 0;
  uint64_t last_compressions_ = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_SWAP_METRICS_DRIVER_MAC_H_
