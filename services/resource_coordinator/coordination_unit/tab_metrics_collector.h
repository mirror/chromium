// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_METRICS_COLLECTOR_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_METRICS_COLLECTOR_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"

namespace resource_coordinator {

class CoordinationUnitImpl;
class FrameCoordinationUnitImpl;

// A TabMetricsCollector observes changes at frame-level, and records metrics.
class TabMetricsCollector : public CoordinationUnitGraphObserver {
 public:
  TabMetricsCollector();
  ~TabMetricsCollector() override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitImpl* coordination_unit) override;
  void OnPropertyChanged(const CoordinationUnitImpl* coordination_unit,
                         const mojom::PropertyType property_type,
                         const base::Value& value) override;

 private:
  struct FrameData {
    base::TimeTicks last_blurt_time_;
    base::TimeTicks last_invisible_time_;
  };

  void OnFramePropertyChanged(
      const FrameCoordinationUnitImpl* coordination_unit,
      const mojom::PropertyType property_type,
      const base::Value& value);

  std::map<CoordinationUnitID, FrameData> frame_data_map_;
  DISALLOW_COPY_AND_ASSIGN(TabMetricsCollector);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_METRICS_COLLECTOR_H_
