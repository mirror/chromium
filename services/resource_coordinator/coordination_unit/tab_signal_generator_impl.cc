// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_signal_generator_impl.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace resource_coordinator {

namespace {

const char* kCPUUsageMetricsName = "CPUUsage";
const size_t kMaxCPUUsageMeasurements = 30u;

}  // namespace

#define DISPATCH_TAB_SIGNAL(observers, METHOD, cu, ...)           \
  observers.ForAllPtrs([cu](mojom::TabSignalObserver* observer) { \
    observer->METHOD(cu->id(), __VA_ARGS__);                      \
  });

TabSignalGeneratorImpl::TabCPUUsageCollector::TabCPUUsageCollector()
    : max_cpu_usage_measurements_(kMaxCPUUsageMeasurements) {
  CheckForFieldTrialParamOverrides();
}

TabSignalGeneratorImpl::TabCPUUsageCollector::~TabCPUUsageCollector() = default;

void TabSignalGeneratorImpl::TabCPUUsageCollector::CollectCPUUsage(
    double cpu_usage) {
  DCHECK(ukm_entry_builder_);
  // Each |cpu_usage| measurement is assigned a metric name that corresponds
  // to the |cpu_measurement_ticks_| value it was observed at.
  ukm_entry_builder_->AddMetric(
      base::Uint64ToString(num_cpu_usage_measurements_++).c_str(),
      static_cast<int>(cpu_usage));
}

bool TabSignalGeneratorImpl::TabCPUUsageCollector::IsCollectingCPUUsage()
    const {
  return ukm_entry_builder_ &&
         num_cpu_usage_measurements_ < max_cpu_usage_measurements_;
}

void TabSignalGeneratorImpl::TabCPUUsageCollector::ResetUkmEntryBuilder(
    std::unique_ptr<ukm::UkmEntryBuilder> ukm_entry_builder) {
  ukm_entry_builder_ = std::move(ukm_entry_builder);
  num_cpu_usage_measurements_ = 0u;
}

void TabSignalGeneratorImpl::TabCPUUsageCollector::
    CheckForFieldTrialParamOverrides() {
  int64_t interval_ms = GetGRCRenderProcessCPUProfilingIntervalMs();
  int64_t duration_ms = GetGRCRenderProcessCPUProfilingDurationMs();

  if (interval_ms > 0 && duration_ms > 0 && duration_ms >= interval_ms) {
    max_cpu_usage_measurements_ =
        static_cast<size_t>(duration_ms / interval_ms);
  }
}

TabSignalGeneratorImpl::TabSignalGeneratorImpl() = default;

TabSignalGeneratorImpl::~TabSignalGeneratorImpl() = default;

void TabSignalGeneratorImpl::AddObserver(mojom::TabSignalObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

bool TabSignalGeneratorImpl::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  auto coordination_unit_type = coordination_unit->id().type;
  return coordination_unit_type == CoordinationUnitType::kWebContents ||
         coordination_unit_type == CoordinationUnitType::kFrame;
}

void TabSignalGeneratorImpl::OnPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  if (coordination_unit->id().type == CoordinationUnitType::kFrame) {
    OnFramePropertyChanged(
        CoordinationUnitImpl::ToFrameCoordinationUnit(coordination_unit),
        property_type, value);
  }
  if (coordination_unit->id().type == CoordinationUnitType::kWebContents) {
    OnTabPropertyChanged(coordination_unit, property_type, value);
  }
}

void TabSignalGeneratorImpl::BindToInterface(
    resource_coordinator::mojom::TabSignalGeneratorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void TabSignalGeneratorImpl::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  if (property_type == mojom::PropertyType::kNetworkIdle) {
    // Ignore when the signal doesn't come from main frame.
    if (!coordination_unit->IsMainFrame())
      return;
    // TODO(lpy) Combine CPU usage or long task idleness signal.
    for (auto* parent : coordination_unit->parents()) {
      if (parent->id().type != CoordinationUnitType::kWebContents)
        continue;
      DISPATCH_TAB_SIGNAL(observers_, OnEventReceived, parent,
                          mojom::TabEvent::kDoneLoading);
      break;
    }
  }
}

void TabSignalGeneratorImpl::OnBeforeCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  if (coordination_unit->id().type == CoordinationUnitType::kWebContents) {
    OnBeforeTabCoordinationUnitDestroyed(coordination_unit);
  }
}

void TabSignalGeneratorImpl::OnBeforeTabCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  tab_cpu_usage_collector_map_.erase(coordination_unit->id());
}

void TabSignalGeneratorImpl::OnTabPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  TabCPUUsageCollector& tab_cpu_usage_collector =
      tab_cpu_usage_collector_map_[coordination_unit->id()];
  if (property_type == mojom::PropertyType::kCPUUsage) {
    if (tab_cpu_usage_collector.IsCollectingCPUUsage()) {
      tab_cpu_usage_collector.CollectCPUUsage(value.GetDouble());
    }
  } else if (property_type == mojom::PropertyType::kUkmSourceId) {
    ukm::SourceId ukm_source_id;
    // |mojom::PropertyType::kUkmSourceId| is stored as a string because
    // |ukm::SourceId is not supported by the coordination unit property
    // store, so it must  be converted before being used.
    DCHECK(base::StringToInt64(value.GetString(), &ukm_source_id));

    tab_cpu_usage_collector.ResetUkmEntryBuilder(
        coordination_unit_manager()->CreateUkmEntryBuilder(
            ukm_source_id, kCPUUsageMetricsName));
  }
}

}  // namespace resource_coordinator
