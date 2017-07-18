// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_signal_generator_impl.h"

#include "base/metrics/histogram_macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"

// From 1 hour to 24 hours, with 100 buckets.
#define HEURISTICS_HISTOGRAM(name, sample)                                \
  UMA_HISTOGRAM_CUSTOM_TIMES(name, sample, base::TimeDelta::FromHours(1), \
                             base::TimeDelta::FromHours(24), 100)

namespace resource_coordinator {

#define DISPATCH_TAB_SIGNAL(observers, METHOD, cu, ...)           \
  observers.ForAllPtrs([cu](mojom::TabSignalObserver* observer) { \
    observer->METHOD(cu->id(), __VA_ARGS__);                      \
  });

constexpr base::TimeDelta kMaxSlientTime = base::TimeDelta::FromMinutes(1);

const char* kBackgroundAudioStartsUMA =
    "TabManager.Heuristics.BackgroundAudioStarts";

TabSignalGeneratorImpl::TabSignalGeneratorImpl()
    : metrics_collector_(base::MakeUnique<MetricsCollector>()) {}

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

void TabSignalGeneratorImpl::OnBeforeCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  if (coordination_unit->id().type == CoordinationUnitType::kFrame) {
    metrics_collector_->CoordinationUnitRemoved(coordination_unit->id());
  }
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
}

void TabSignalGeneratorImpl::BindToInterface(
    const service_manager::BindSourceInfo& source_info,
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
  metrics_collector_->OnFramePropertyChanged(coordination_unit, property_type,
                                             value);
}

TabSignalGeneratorImpl::MetricsCollector::MetricsCollector() = default;

TabSignalGeneratorImpl::MetricsCollector::~MetricsCollector() = default;

void TabSignalGeneratorImpl::MetricsCollector::CoordinationUnitRemoved(
    CoordinationUnitID cu_id) {
  frame_data_map_.erase(cu_id);
}

void TabSignalGeneratorImpl::MetricsCollector::OnFramePropertyChanged(
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
