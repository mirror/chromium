// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_

#include "base/macros.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"

namespace resource_coordinator {

class CoordinationUnitImpl;
class FrameCoordinationUnitImpl;

extern const char kTabFromBackgroundedToFirstAudioStartsUMA[];

// A MetricsCollector observes changes happened inside CoordinationUnit Graph,
// and reports UMA/UKM.
class MetricsCollector : public CoordinationUnitGraphObserver {
 public:
  MetricsCollector();
  ~MetricsCollector() override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitImpl* coordination_unit) override;
  void OnPropertyChanged(const CoordinationUnitImpl* coordination_unit,
                         const mojom::PropertyType property_type,
                         const base::Value& value) override;

 private:
  friend class MetricsCollectorTest;

  struct MetricsReportRecord {
    MetricsReportRecord();
    bool first_audible_after_backgrounded_reported;
  };

  struct FrameData {
    base::TimeTicks last_blurt_time;
    base::TimeTicks last_invisible_time;
  };

  void OnFramePropertyChanged(
      const FrameCoordinationUnitImpl* frame_coordination_unit,
      const mojom::PropertyType property_type,
      const base::Value& value);

  // Note: |clock_| is always |&default_tick_clock_|, except during unit
  // testing.
  base::DefaultTickClock default_tick_clock_;
  base::TickClock* const clock_;
  std::map<CoordinationUnitID, FrameData> frame_data_map_;
  // The metrics_report_record_map_ is used to record whether a metric was
  // already reported to avoid reporting multiple metrics.
  std::map<CoordinationUnitID, MetricsReportRecord> metrics_report_record_map_;
  DISALLOW_COPY_AND_ASSIGN(MetricsCollector);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_
