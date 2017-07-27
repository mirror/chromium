// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/metrics_collector.h"

#include "base/metrics/histogram_macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"

// Tabs can be kept in the background for a long time, metrics show 75th
// percentile of time spent in background is 2.5 hours, and the 95th is 24 hour.
// In order to guide the selection of an appropriate observation window we are
// proposing using a CUSTOM_TIMES histogram from 1s to 48h, with 100 buckets.
#define HEURISTICS_HISTOGRAM(name, sample)                                  \
  UMA_HISTOGRAM_CUSTOM_TIMES(name, sample, base::TimeDelta::FromSeconds(1), \
                             base::TimeDelta::FromHours(48), 100)

namespace resource_coordinator {

constexpr base::TimeDelta kMaxAudioSlientTime = base::TimeDelta::FromMinutes(1);

const char kTabFromBackgroundedToFirstAudioStartsUMA[] =
    "TabManager.Heuristics.FromBackgroundedToFirstAudioStarts";

MetricsCollector::MetricsCollector() : clock_(&default_tick_clock_) {}

MetricsCollector::~MetricsCollector() = default;

bool MetricsCollector::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  return coordination_unit->id().type == CoordinationUnitType::kFrame ||
         coordination_unit->id().type == CoordinationUnitType::kWebContents;
}

void MetricsCollector::OnBeforeCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  if (coordination_unit->id().type == CoordinationUnitType::kFrame) {
    frame_data_map_.erase(coordination_unit->id());
  } else if (coordination_unit->id().type ==
             CoordinationUnitType::kWebContents) {
    metrics_report_record_map_.erase(coordination_unit->id());
  }
}

void MetricsCollector::OnPropertyChanged(
    const CoordinationUnitImpl* coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  if (coordination_unit->id().type == CoordinationUnitType::kFrame) {
    OnFramePropertyChanged(
        CoordinationUnitImpl::ToFrameCoordinationUnit(coordination_unit),
        property_type, value);
  }
}

void MetricsCollector::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* frame_coordination_unit,
    const mojom::PropertyType property_type,
    const base::Value& value) {
  FrameData& frame_data = frame_data_map_[frame_coordination_unit->id()];
  if (property_type == mojom::PropertyType::kAudible) {
    bool audible = value.GetBool();
    if (!audible) {
      frame_data.last_blurt_time = clock_->NowTicks();
      return;
    }
    auto* web_contents_coordination_unit =
        frame_coordination_unit->GetWebContentsCoordinationUnit();
    // Only record metrics while it is backgrounded.
    if (!web_contents_coordination_unit ||
        web_contents_coordination_unit->IsVisible())
      return;
    // Audio is considered to have started playing if the tab has never
    // previously played audio, or has been silent for at least one minute.
    auto now = clock_->NowTicks();
    if (frame_data.last_blurt_time + kMaxAudioSlientTime < now) {
      MetricsReportRecord& record =
          metrics_report_record_map_[web_contents_coordination_unit->id()];
      if (!record.first_audible_after_backgrounded_reported) {
        HEURISTICS_HISTOGRAM(kTabFromBackgroundedToFirstAudioStartsUMA,
                             now - frame_data.last_invisible_time);
        record.first_audible_after_backgrounded_reported = true;
      }
    }
  } else if (property_type == mojom::PropertyType::kVisible) {
    bool visible = value.GetBool();
    if (visible)
      return;
    frame_data.last_invisible_time = clock_->NowTicks();
  }
}

MetricsCollector::MetricsReportRecord::MetricsReportRecord() {
  first_audible_after_backgrounded_reported = false;
}

}  // namespace resource_coordinator
