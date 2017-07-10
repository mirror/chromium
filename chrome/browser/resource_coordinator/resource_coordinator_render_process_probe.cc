// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/resource_coordinator_render_process_probe.h"

#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_interface.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

#if defined(OS_MACOSX)
#include "content/public/browser/browser_child_process_host.h"
#endif

namespace resource_coordinator {

namespace {

const int kDefaultMeasurementIntervalInSeconds = 1;

base::LazyInstance<ResourceCoordinatorRenderProcessProbe>::DestructorAtExit
    g_probe = LAZY_INSTANCE_INITIALIZER;

}  // namespace

ResourceCoordinatorRenderProcessProbe::RenderProcessInfo::RenderProcessInfo() =
    default;

ResourceCoordinatorRenderProcessProbe::RenderProcessInfo::~RenderProcessInfo() =
    default;

ResourceCoordinatorRenderProcessProbe::ResourceCoordinatorRenderProcessProbe()
    : measurement_interval_msec_(
          base::TimeDelta::FromSeconds(kDefaultMeasurementIntervalInSeconds)) {}

ResourceCoordinatorRenderProcessProbe::
    ~ResourceCoordinatorRenderProcessProbe() = default;

// static
ResourceCoordinatorRenderProcessProbe*
ResourceCoordinatorRenderProcessProbe::GetInstance() {
  return g_probe.Pointer();
}

// static
bool ResourceCoordinatorRenderProcessProbe::IsEnabled() {
  return base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator) &&
         base::FeatureList::IsEnabled(features::kGRCRenderProcessCPUProfiling);
}

void ResourceCoordinatorRenderProcessProbe::StartGatherCycle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  repeating_timer_.Start(FROM_HERE, base::TimeDelta(), this,
                         &ResourceCoordinatorRenderProcessProbe::
                             RegisterAliveRenderProcessesOnUIThread);
}

void ResourceCoordinatorRenderProcessProbe::
    RegisterAliveRenderProcessesOnUIThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  ++current_measurement_tick_;

  for (content::RenderProcessHost::iterator rph_iter =
           content::RenderProcessHost::AllHostsIterator();
       !rph_iter.IsAtEnd(); rph_iter.Advance()) {
    content::RenderProcessHost* host = rph_iter.GetCurrentValue();
    base::ProcessHandle handle = host->GetHandle();
    // Process may not be valid yet.
    if (handle == base::kNullProcessHandle) {
      continue;
    }

    auto& render_process_info = render_process_info_[handle];
    render_process_info.last_measurement_tick_active =
        current_measurement_tick_;

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
      base::BindOnce(&ResourceCoordinatorRenderProcessProbe::
                         CollectRenderProcessMetricsOnIOThread,
                     base::Unretained(this)));
}

void ResourceCoordinatorRenderProcessProbe::
    CollectRenderProcessMetricsOnIOThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  RenderProcessInfoMap::iterator iter = render_process_info_.begin();
  while (iter != render_process_info_.end()) {
    auto& render_process_info = iter->second;

    if (render_process_info.last_measurement_tick_active ==
        current_measurement_tick_) {
      render_process_info.cpu_usage =
          render_process_info.metrics->GetCPUUsage();
      LOG(ERROR) << "CPU USAGE IS "
                 << std::to_string(render_process_info.cpu_usage);
      ++iter;
    } else {
      render_process_info_.erase(iter++);
      continue;
    }
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &ResourceCoordinatorRenderProcessProbe::
              SendRenderProcessMetricsToResourceCoordinatorServiceOnUIThread,
          base::Unretained(this)));
}

void ResourceCoordinatorRenderProcessProbe::
    SendRenderProcessMetricsToResourceCoordinatorServiceOnUIThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  for (auto& render_process_info_entry : render_process_info_) {
    auto& render_process_info = render_process_info_entry.second;
    render_process_info.host->GetProcessResourceCoordinator()->SetProperty(
        mojom::PropertyType::kCPUUsage,
        base::MakeUnique<base::Value>(render_process_info.cpu_usage));
  }

  repeating_timer_.Start(FROM_HERE, measurement_interval_msec_, this,
                         &ResourceCoordinatorRenderProcessProbe::
                             RegisterAliveRenderProcessesOnUIThread);
}

}  // namespace resource_coordinator
