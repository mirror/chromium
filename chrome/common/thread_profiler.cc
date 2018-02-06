// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/thread_profiler.h"

#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequence_local_storage_slot.h"
#include "chrome/common/stack_sampling_configuration.h"
#include "components/metrics/call_stack_profile_metrics_provider.h"
#include "components/metrics/call_stack_profile_params.h"
#include "components/metrics/child_call_stack_profile_collector.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Only used by child processes.
base::LazyInstance<metrics::ChildCallStackProfileCollector>::Leaky
    g_child_call_stack_profile_collector = LAZY_INSTANCE_INITIALIZER;

// The profiler object is stored in a SequenceLocalStorageSlot on child threads
// to give it the same lifetime as the threads.
base::LazyInstance<
    base::SequenceLocalStorageSlot<std::unique_ptr<ThreadProfiler>>>::Leaky
    g_child_thread_profiler = LAZY_INSTANCE_INITIALIZER;

constexpr const base::StackSamplingProfiler::SamplingParams
    startup_sampling_params = {
        .initial_delay = base::TimeDelta::FromMilliseconds(0),
        .bursts = 1,
        .samples_per_burst = 300,
        .sampling_interval = base::TimeDelta::FromMilliseconds(100)};

metrics::CallStackProfileParams::Process GetProcess() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  const std::map<std::string, metrics::CallStackProfileParams::Process>
      process_map = {
          {"", metrics::CallStackProfileParams::BROWSER_PROCESS},
          {switches::kRendererProcess,
           metrics::CallStackProfileParams::RENDERER_PROCESS},
          {switches::kGpuProcess, metrics::CallStackProfileParams::GPU_PROCESS},
          {switches::kUtilityProcess,
           metrics::CallStackProfileParams::UTILITY_PROCESS},
          {switches::kZygoteProcess,
           metrics::CallStackProfileParams::ZYGOTE_PROCESS},
          {switches::kPpapiPluginProcess,
           metrics::CallStackProfileParams::PPAPI_PLUGIN_PROCESS},
          {switches::kPpapiBrokerProcess,
           metrics::CallStackProfileParams::PPAPI_BROKER_PROCESS}};
  const auto loc = process_map.find(process_type);
  if (loc != process_map.end())
    return loc->second;
  return metrics::CallStackProfileParams::UNKNOWN_PROCESS;
}

}  // namespace

ThreadProfiler::~ThreadProfiler() {}

// static
std::unique_ptr<ThreadProfiler> ThreadProfiler::CreateAndStartOnMainThread(
    metrics::CallStackProfileParams::Thread thread) {
  return std::unique_ptr<ThreadProfiler>(new ThreadProfiler(thread));
}

// static
void ThreadProfiler::StartOnChildThread(
    metrics::CallStackProfileParams::Thread thread) {
  auto profiler = std::unique_ptr<ThreadProfiler>(new ThreadProfiler(thread));
  g_child_thread_profiler.Get().Set(std::move(profiler));
}

void ThreadProfiler::SetServiceManagerConnectorForChildProcess(
    service_manager::Connector* connector) {
  DCHECK_NE(metrics::CallStackProfileParams::BROWSER_PROCESS, GetProcess());
  metrics::mojom::CallStackProfileCollectorPtr browser_interface;
  connector->BindInterface(content::mojom::kBrowserServiceName,
                           &browser_interface);
  g_child_call_stack_profile_collector.Get().SetParentProfileCollector(
      std::move(browser_interface));
}

ThreadProfiler::ThreadProfiler(metrics::CallStackProfileParams::Thread thread)
    : startup_profile_params_(GetProcess(),
                              thread,
                              metrics::CallStackProfileParams::PROCESS_STARTUP,
                              metrics::CallStackProfileParams::MAY_SHUFFLE) {
  if (!StackSamplingConfiguration::Get()->IsProfilerEnabledForCurrentProcess())
    return;

  startup_profiler_ = std::make_unique<base::StackSamplingProfiler>(
      base::PlatformThread::CurrentId(), startup_sampling_params,
      BindRepeating(&ThreadProfiler::ReceiveStartupProfiles,
                    GetReceiverCallback(&startup_profile_params_)));
  startup_profiler_->Start();
}

base::StackSamplingProfiler::CompletedCallback
ThreadProfiler::GetReceiverCallback(
    metrics::CallStackProfileParams* profile_params) {
  profile_params->start_timestamp = base::TimeTicks::Now();
  // TODO(wittman): Simplify the approach to getting the profiler callback
  // across CallStackProfileMetricsProvider and
  // ChildCallStackProfileCollector. Ultimately both should expose functions
  // like
  //
  //   void ReceiveCompletedProfiles(
  //       const metrics::CallStackProfileParams& profile_params,
  //       base::TimeTicks profile_start_time,
  //       base::StackSamplingProfiler::CallStackProfiles profiles);
  //
  // and this function should bind the passed profile_params and
  // base::TimeTicks::Now() to those functions.
  if (GetProcess() == metrics::CallStackProfileParams::BROWSER_PROCESS) {
    return metrics::CallStackProfileMetricsProvider::
        GetProfilerCallbackForBrowserProcess(profile_params);
  }
  return g_child_call_stack_profile_collector.Get()
      .ChildCallStackProfileCollector::GetProfilerCallback(*profile_params);
}

// static
base::Optional<base::StackSamplingProfiler::SamplingParams>
ThreadProfiler::ReceiveStartupProfiles(
    const base::StackSamplingProfiler::CompletedCallback& receiver_callback,
    base::StackSamplingProfiler::CallStackProfiles profiles) {
  receiver_callback.Run(std::move(profiles));
  return base::Optional<base::StackSamplingProfiler::SamplingParams>();
}
