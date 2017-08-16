// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/resource_coordinator_process_probe.h"

#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/process/process_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_interface.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

#if defined(OS_MACOSX)
#include "content/public/browser/browser_child_process_host.h"
#endif

namespace resource_coordinator {

namespace {

const int kDefaultMeasurementIntervalInSeconds = 1;

base::LazyInstance<ResourceCoordinatorProcessProbe>::DestructorAtExit g_probe =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

ProcessInfo::ProcessInfo() = default;
ProcessInfo::~ProcessInfo() = default;

BrowserProcessInfo::BrowserProcessInfo() = default;
BrowserProcessInfo::~BrowserProcessInfo() = default;

RenderProcessInfo::RenderProcessInfo() = default;
RenderProcessInfo::~RenderProcessInfo() = default;

ProcessMetricsHandler::ProcessMetricsHandler() = default;
ProcessMetricsHandler::~ProcessMetricsHandler() = default;

class ResourceCoordinatorProcessMetricsHandler : public ProcessMetricsHandler {
 public:
  ResourceCoordinatorProcessMetricsHandler() = default;
  ~ResourceCoordinatorProcessMetricsHandler() override = default;

  // Send collected metrics back to the |resource_coordinator| service
  // and initiates another render process metrics gather cycle.
  bool HandleMetrics(
      const BrowserProcessInfo& browser_process_info,
      const RenderProcessInfoMap& render_process_info_map) override {
    double total_cpu_usage = browser_process_info.cpu_usage;
    for (auto& render_process_info_map_entry : render_process_info_map) {
      auto& render_process_info = render_process_info_map_entry.second;
      // TODO(oysteine): Move the multiplier used to avoid precision loss
      // into a shared location, when this property gets used.
      render_process_info.host->GetProcessResourceCoordinator()->SetProperty(
          mojom::PropertyType::kCPUUsage, render_process_info.cpu_usage * 1000);
      total_cpu_usage += render_process_info.cpu_usage;
    }

    g_browser_process->GetTabManager()
        ->tab_manager_stats_collector_->RecordCPUUsage(total_cpu_usage);

    return true;
  }
};

ResourceCoordinatorProcessProbe::ResourceCoordinatorProcessProbe()
    : metrics_handler_(
          base::MakeUnique<ResourceCoordinatorProcessMetricsHandler>()),
      interval_ms_(
          base::TimeDelta::FromSeconds(kDefaultMeasurementIntervalInSeconds)) {
  UpdateWithFieldTrialParams();
}

ResourceCoordinatorProcessProbe::~ResourceCoordinatorProcessProbe() = default;

// static
ResourceCoordinatorProcessProbe*
ResourceCoordinatorProcessProbe::GetInstance() {
  return g_probe.Pointer();
}

// static
bool ResourceCoordinatorProcessProbe::IsEnabled() {
  // Check that service_manager is active, GRC is enabled,
  // and render process CPU profiling is enabled.
  return content::ServiceManagerConnection::GetForProcess() != nullptr &&
         resource_coordinator::IsResourceCoordinatorEnabled() &&
         base::FeatureList::IsEnabled(features::kGRCProcessCPUProfiling);
}

void ResourceCoordinatorProcessProbe::StartGatherCycle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!ResourceCoordinatorProcessProbe::IsEnabled()) {
    return;
  }

  timer_.Start(
      FROM_HERE, base::TimeDelta(), this,
      &ResourceCoordinatorProcessProbe::RegisterAliveProcessesOnUIThread);
}

void ResourceCoordinatorProcessProbe::RegisterAliveProcessesOnUIThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (browser_process_info_.metrics.get() == nullptr)
    browser_process_info_.metrics = base::ProcessMetrics::CreateProcessMetrics(
        base::GetCurrentProcessHandle());

  ++current_gather_cycle_;

  for (content::RenderProcessHost::iterator rph_iter =
           content::RenderProcessHost::AllHostsIterator();
       !rph_iter.IsAtEnd(); rph_iter.Advance()) {
    content::RenderProcessHost* host = rph_iter.GetCurrentValue();
    base::ProcessHandle handle = host->GetHandle();
    // Process may not be valid yet.
    if (handle == base::kNullProcessHandle) {
      continue;
    }

    auto& render_process_info = render_process_info_map_[handle];
    render_process_info.last_gather_cycle_active = current_gather_cycle_;

    if (render_process_info.metrics.get() == nullptr) {
#if defined(OS_MACOSX)
      render_process_info.metrics = base::ProcessMetrics::CreateProcessMetrics(
          handle, content::BrowserChildProcessHost::GetPortProvider());
#else
      render_process_info.metrics =
          base::ProcessMetrics::CreateProcessMetrics(handle);
#endif
      render_process_info.host = host;
    }
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &ResourceCoordinatorProcessProbe::CollectProcessMetricsOnIOThread,
          base::Unretained(this)));
}

void ResourceCoordinatorProcessProbe::CollectProcessMetricsOnIOThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  browser_process_info_.cpu_usage =
      browser_process_info_.metrics->GetPlatformIndependentCPUUsage();

  RenderProcessInfoMap::iterator iter = render_process_info_map_.begin();
  while (iter != render_process_info_map_.end()) {
    auto& render_process_info = iter->second;

    // If the last gather cycle the render process was marked as active is
    // not current then it is assumed dead and should not be measured anymore.
    if (render_process_info.last_gather_cycle_active == current_gather_cycle_) {
      render_process_info.cpu_usage =
          render_process_info.metrics->GetPlatformIndependentCPUUsage();
      ++iter;
    } else {
      render_process_info_map_.erase(iter++);
      continue;
    }
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &ResourceCoordinatorProcessProbe::HandleProcessMetricsOnUIThread,
          base::Unretained(this)));
}

void ResourceCoordinatorProcessProbe::HandleProcessMetricsOnUIThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (metrics_handler_->HandleMetrics(browser_process_info_,
                                      render_process_info_map_)) {
    timer_.Start(
        FROM_HERE, interval_ms_, this,
        &ResourceCoordinatorProcessProbe::RegisterAliveProcessesOnUIThread);
  }
}

bool ResourceCoordinatorProcessProbe::
    AllRenderProcessMeasurementsAreCurrentForTesting() const {
  for (auto& render_process_info_map_entry : render_process_info_map_) {
    auto& render_process_info = render_process_info_map_entry.second;
    if (render_process_info.last_gather_cycle_active != current_gather_cycle_ ||
        render_process_info.cpu_usage < 0.0) {
      return false;
    }
  }
  return true;
}

void ResourceCoordinatorProcessProbe::UpdateWithFieldTrialParams() {
  int64_t interval_ms = GetGRCProcessCPUProfilingIntervalInMs();

  if (interval_ms > 0) {
    interval_ms_ = base::TimeDelta::FromMilliseconds(interval_ms);
  }
}

}  // namespace resource_coordinator
