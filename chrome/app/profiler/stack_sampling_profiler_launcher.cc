// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/profiler/stack_sampling_profiler_launcher.h"

#include "base/command_line.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/threading/platform_thread.h"
#include "chrome/common/stack_sampling_configuration.h"
#include "components/metrics/call_stack_profile_metrics_provider.h"
#include "components/metrics/call_stack_profile_params.h"
#include "components/metrics/child_call_stack_profile_collector.h"
#include "content/public/common/content_switches.h"

namespace {

// Returns the profiler callback appropriate for the current process.
base::StackSamplingProfiler::CompletedCallback GetProfilerCallback() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);

  // The browser process has an empty process type.
  if (process_type.empty()) {
    return metrics::CallStackProfileMetricsProvider::
        GetProfilerCallbackForBrowserProcessStartup();
  }

  if (process_type == switches::kGpuProcess) {
    return metrics::ChildCallStackProfileCollector::Get()->GetProfilerCallback(
        metrics::CallStackProfileParams(
            metrics::CallStackProfileParams::GPU_PROCESS,
            metrics::CallStackProfileParams::GPU_MAIN_THREAD,
            metrics::CallStackProfileParams::PROCESS_STARTUP,
            metrics::CallStackProfileParams::MAY_SHUFFLE));
  }

  // No other processes are currently supported.
  NOTREACHED();
  return base::StackSamplingProfiler::CompletedCallback();
}

}  // namespace

void StartStackSamplingProfilerIfEnabled() {
  StackSamplingConfiguration* config = StackSamplingConfiguration::Get();
  if (!config->IsProfilerEnabledForCurrentProcess())
    return;

  // Create on access, so that initialization order is well-defined. There is no
  // need to use a LazyInstance, as the profiler is guaranteed to be created
  // while Chrome is still running single-threaded.
  static base::StackSamplingProfiler sampling_profiler(
      base::PlatformThread::CurrentId(),
      config->GetSamplingParamsForCurrentProcess(), GetProfilerCallback());
  sampling_profiler.Start();
}
