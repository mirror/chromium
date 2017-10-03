// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/profiler/main_process_stack_sampling_profiler.h"

#include "base/profiler/stack_sampling_profiler.h"
#include "base/threading/platform_thread.h"
#include "chrome/common/stack_sampling_configuration.h"
#include "components/metrics/call_stack_profile_metrics_provider.h"

void StartMainProcessStackSamplingProfilerIfEnabled() {
  StackSamplingConfiguration* config = StackSamplingConfiguration::Get();
  if (!config->IsProfilerEnabledForCurrentProcess())
    return;

  // Create on access, so that initialization order is well-defined. There is no
  // need to use a LazyInstance, as the profiler is guaranteed to be created
  // while Chrome is still running single-threaded.
  static base::StackSamplingProfiler sampling_profiler(
      base::PlatformThread::CurrentId(),
      config->GetSamplingParamsForCurrentProcess(),
      metrics::CallStackProfileMetricsProvider::
          GetProfilerCallbackForBrowserProcessStartup());
  sampling_profiler.Start();
}
