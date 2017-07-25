// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_METRICS_COLLECTOR_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_METRICS_COLLECTOR_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_manager.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/interfaces/tab_signal.mojom.h"

namespace resource_coordinator {

class CoordinationUnitImpl;
class WebContentsCoordinationUnitImpl;

// The TabMetricsCollector is a dedicated |CoordinationUnitGraphObserver| for
// aggregating tab-related metrics for reporting purposes.
class TabMetricsCollector : public CoordinationUnitGraphObserver {
 public:
  TabMetricsCollector();
  ~TabMetricsCollector() override;

  // CoordinationUnitGraphObserver implementation.
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitImpl* coordination_unit) override;
  void OnPropertyChanged(const CoordinationUnitImpl* coordination_unit,
                         const mojom::PropertyType property_type,
                         const base::Value& value) override;
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;

 private:
  class UkmTabCPUUsageCollector {
   public:
    UkmTabCPUUsageCollector();
    ~UkmTabCPUUsageCollector();

    void CollectCPUUsage(double cpu_usage);
    bool IsCollectingCPUUsage() const;
    void ResetUkmEntryBuilder(
        std::unique_ptr<ukm::UkmEntryBuilder> ukm_entry_builder_);

    void set_num_coresident_tabs(size_t num_coresident_tabs) {
      num_coresident_tabs_ = num_coresident_tabs;
    }
    size_t num_coresident_tabs() { return num_coresident_tabs_; }

   private:
    size_t max_cpu_usage_measurements_ = 0u;
    size_t num_cpu_usage_measurements_ = 0u;
    size_t num_coresident_tabs_ = 0u;
    std::unique_ptr<ukm::UkmEntryBuilder> ukm_entry_builder_;

    // Allows FieldTrial parameters to override defaults.
    void UpdateWithFieldTrialParams();

    DISALLOW_COPY_AND_ASSIGN(UkmTabCPUUsageCollector);
  };

  void OnBeforeTabCoordinationUnitDestroyed(
      const WebContentsCoordinationUnitImpl* web_contents_coordination_unit);
  void OnTabPropertyChanged(
      const WebContentsCoordinationUnitImpl* web_contents_coordination_unit,
      const mojom::PropertyType property_type,
      const base::Value& value);

  std::map<CoordinationUnitID, UkmTabCPUUsageCollector>
      ukm_tab_cpu_usage_collector_map_;

  DISALLOW_COPY_AND_ASSIGN(TabMetricsCollector);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_METRICS_COLLECTOR_H_
