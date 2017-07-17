// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_metrics_collector.h"

#include "base/metrics/histogram_macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"

// From 1 hour to 24 hours, with 100 buckets.
#define HEURISTICS_HISTOGRAM(name, sample)                                \
  UMA_HISTOGRAM_CUSTOM_TIMES(name, sample, base::TimeDelta::FromHours(1), \
                             base::TimeDelta::FromHours(24), 100)

namespace resource_coordinator {

constexpr base::TimeDelta kMaxSlientTime = base::TimeDelta::FromMinutes(1);

const char* kBackgroundAudioStartsUMA =
    "TabManager.Heuristics.BackgroundAudioStarts";

TabMetricsCollector::TabMetricsCollector() = default;

TabMetricsCollector::~TabMetricsCollector() = default;

bool TabMetricsCollector::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  return coordination_unit->id().type == CoordinationUnitType::kFrame;
}

void TabMetricsCollector::OnBeforeCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  frame_data_map_.erase(coordination_unit->id());
}

void TabMetricsCollector::OnPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  OnFramePropertyChanged(
      CoordinationUnitImpl::ToFrameCoordinationUnit(coordination_unit),
      property_type, value);
}

void TabMetricsCollector::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  FrameData& frame_data = frame_data_map_[coordination_unit->id()];
  if (property_type == mojom::PropertyType::kAudible) {
    bool audible = value.GetBool();
    if (!audible) {
      frame_data.last_blurt_time_ = base::TimeTicks::Now();
      return;
    }
    auto visible_value =
        coordination_unit->GetProperty(mojom::PropertyType::kVisible);
    if (!visible_value.is_bool())
      return;
    bool visible = visible_value.GetBool();
    // Only record metrics while it is backgrounded.
    if (visible)
      return;
    // Audio is considered to have started playing if the tab has never
    // previously played audio, or has been silent for at least one minute.
    auto now = base::TimeTicks::Now();
    if (frame_data.last_blurt_time_ + kMaxSlientTime < now) {
      HEURISTICS_HISTOGRAM(kBackgroundAudioStartsUMA,
                           now - frame_data.last_invisible_time_);
    }
  } else if (property_type == mojom::PropertyType::kVisible) {
    bool visible = value.GetBool();
    if (visible)
      return;
    frame_data.last_invisible_time_ = base::TimeTicks::Now();
  }
}

}  // namespace resource_coordinator
