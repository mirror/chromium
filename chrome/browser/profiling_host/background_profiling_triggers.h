// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILING_HOST_BACKGROUND_PROFILING_TRIGGERS_H_
#define CHROME_BROWSER_PROFILING_HOST_BACKGROUND_PROFILING_TRIGGERS_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

namespace profiling {

class BackgroundProfilingTriggers {
 public:
  ~BackgroundProfilingTriggers();
  BackgroundProfilingTriggers();

 private:
  // Check the current memory usage and send a slow-report if needed.
  void PerformMemoryUsageChecks();
  void PerformMemoryUsageChecksOnIOThread();

  void OnReceivedMemoryDump(
      bool success,
      memory_instrumentation::mojom::GlobalMemoryDumpPtr ptr);

  // Timer to periodically check memory consumption and upload a slow-report.
  base::RepeatingTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundProfilingTriggers);
};

}  // namespace profiling

#endif  // CHROME_BROWSER_PROFILING_HOST_BACKGROUND_PROFILING_TRIGGERS_H_
