// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_SIGNAL_GENERATOR_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_SIGNAL_GENERATOR_IMPL_H_

#include "base/macros.h"
#include "base/time/default_tick_clock.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/interfaces/tab_signal.mojom.h"

namespace resource_coordinator {

class CoordinationUnitImpl;
class FrameCoordinationUnitImpl;

// The TabSignalGenerator is a dedicated |CoordinationUnitGraphObserver| for
// calculating and emitting tab-scoped signals. This observer observes Tab
// CoordinationUnits and Frame CoordinationUnits, utilize information from the
// graph and generate tab level signals.
class TabSignalGeneratorImpl : public CoordinationUnitGraphObserver,
                               public mojom::TabSignalGenerator {
 public:
  TabSignalGeneratorImpl();
  ~TabSignalGeneratorImpl() override;

  // mojom::SignalGenerator implementation.
  void AddObserver(mojom::TabSignalObserverPtr observer) override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitImpl* coordination_unit) override;
  void OnPropertyChanged(const CoordinationUnitImpl* coordination_unit,
                         const mojom::PropertyType property_type,
                         const base::Value& value) override;

  void BindToInterface(
      resource_coordinator::mojom::TabSignalGeneratorRequest request);

 private:
  friend class TabSignalGeneratorTest;

  class MetricsCollector {
   public:
    MetricsCollector();
    ~MetricsCollector();
    void CoordinationUnitRemoved(CoordinationUnitID);
    void OnFramePropertyChanged(
        const FrameCoordinationUnitImpl* coordination_unit,
        const mojom::PropertyType property_type,
        const base::Value& value);

   private:
    friend class TabSignalGeneratorTest;

    struct FrameData {
      base::TimeTicks last_blurt_time_;
      base::TimeTicks last_invisible_time_;
    };

    // Note: |clock_| is always |&default_tick_clock_|, except during unit
    // testing.
    base::DefaultTickClock default_tick_clock_;
    base::TickClock* const clock_;
    std::map<CoordinationUnitID, FrameData> frame_data_map_;

    DISALLOW_COPY_AND_ASSIGN(MetricsCollector);
  };

  void OnFramePropertyChanged(
      const FrameCoordinationUnitImpl* coordination_unit,
      const mojom::PropertyType property_type,
      const base::Value& value);

  MetricsCollector* metrics_collector_for_testing() {
    return metrics_collector_.get();
  }

  std::unique_ptr<MetricsCollector> metrics_collector_;
  mojo::BindingSet<mojom::TabSignalGenerator> bindings_;
  mojo::InterfacePtrSet<mojom::TabSignalObserver> observers_;
  DISALLOW_COPY_AND_ASSIGN(TabSignalGeneratorImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_TAB_SIGNAL_GENERATOR_IMPL_H_
