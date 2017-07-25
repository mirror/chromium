// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_metrics_collector.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace resource_coordinator {

namespace {

const char* kCPUUsageMetricsName = "CPUUsage";
const char* kNumCoresidentTabsEntryName = "NumberOfCoresidentTabs";
const size_t kMaxCPUUsageMeasurements = 30u;

// Gets the number of tabs that are co-resident in all of the render processes
// associated with a |CoordinationUnitType::kWebContents| coordination unit.
size_t GetNumCoresidentTabs(const CoordinationUnitImpl* coordination_unit) {
  DCHECK_EQ(CoordinationUnitType::kWebContents, coordination_unit->id().type);
  std::set<CoordinationUnitImpl*> coresident_tabs;
  for (auto* process_coordination_unit :
       coordination_unit->GetAssociatedCoordinationUnitsOfType(
           CoordinationUnitType::kProcess)) {
    for (auto* tab_coordination_unit :
         process_coordination_unit->GetAssociatedCoordinationUnitsOfType(
             CoordinationUnitType::kWebContents)) {
      coresident_tabs.insert(tab_coordination_unit);
    }
  }
  // A tab cannot be co-resident with itself.
  return coresident_tabs.size() - 1;
}

}  // namespace

TabMetricsCollector::TabCPUUsageCollector::TabCPUUsageCollector()
    : max_cpu_usage_measurements_(kMaxCPUUsageMeasurements) {
  UpdateWithFieldTrialParams();
}

TabMetricsCollector::TabCPUUsageCollector::~TabCPUUsageCollector() {
  ukm_entry_builder_->AddMetric(kNumCoresidentTabsEntryName,
                                static_cast<int>(num_coresident_tabs_));
}

void TabMetricsCollector::TabCPUUsageCollector::CollectCPUUsage(
    double cpu_usage) {
  DCHECK(ukm_entry_builder_);
  // Each |cpu_usage| measurement is assigned a metric name that corresponds
  // to the |num_cpu_usage_measurements_| value it was observed at.
  ukm_entry_builder_->AddMetric(
      base::Uint64ToString(num_cpu_usage_measurements_++).c_str(),
      static_cast<int>(cpu_usage));
}

bool TabMetricsCollector::TabCPUUsageCollector::IsCollectingCPUUsage() const {
  return ukm_entry_builder_ &&
         num_cpu_usage_measurements_ < max_cpu_usage_measurements_;
}

void TabMetricsCollector::TabCPUUsageCollector::ResetUkmEntryBuilder(
    std::unique_ptr<ukm::UkmEntryBuilder> ukm_entry_builder) {
  ukm_entry_builder_ = std::move(ukm_entry_builder);
  num_cpu_usage_measurements_ = 0u;
}

void TabMetricsCollector::TabCPUUsageCollector::UpdateWithFieldTrialParams() {
  int64_t interval_ms = GetGRCRenderProcessCPUProfilingIntervalMs();
  int64_t duration_ms = GetGRCRenderProcessCPUProfilingDurationMs();

  if (interval_ms > 0 && duration_ms > 0 && duration_ms >= interval_ms) {
    max_cpu_usage_measurements_ =
        static_cast<size_t>(duration_ms / interval_ms);
  }
}

TabMetricsCollector::TabMetricsCollector() = default;

TabMetricsCollector::~TabMetricsCollector() = default;

bool TabMetricsCollector::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  auto coordination_unit_type = coordination_unit->id().type;
  return coordination_unit_type == CoordinationUnitType::kWebContents;
}

void TabMetricsCollector::OnPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  if (coordination_unit->id().type == CoordinationUnitType::kWebContents) {
    OnTabPropertyChanged(coordination_unit, property_type, value);
  }
}

void TabMetricsCollector::OnBeforeCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  if (coordination_unit->id().type == CoordinationUnitType::kWebContents) {
    OnBeforeTabCoordinationUnitDestroyed(coordination_unit);
  }
}

void TabMetricsCollector::OnBeforeTabCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  tab_cpu_usage_collector_map_.erase(coordination_unit->id());
}

void TabMetricsCollector::OnTabPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  TabCPUUsageCollector& tab_cpu_usage_collector =
      tab_cpu_usage_collector_map_[coordination_unit->id()];
  if (property_type == mojom::PropertyType::kCPUUsage) {
    if (tab_cpu_usage_collector.IsCollectingCPUUsage()) {
      tab_cpu_usage_collector.CollectCPUUsage(value.GetDouble());
      tab_cpu_usage_collector.set_num_coresident_tabs(
          std::max(tab_cpu_usage_collector.num_coresident_tabs(),
                   GetNumCoresidentTabs(coordination_unit)));
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
