// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_INTROSPECTOR_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_INTROSPECTOR_IMPL_H_

#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_introspector.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace service_manager {
struct BindSourceInfo;
}  // namespace service_manager

namespace resource_coordinator {

class CoordinationUnitIntrospectorImpl
    : public mojom::CoordinationUnitIntrospector,
      public CoordinationUnitGraphObserver {
 public:
  CoordinationUnitIntrospectorImpl();
  ~CoordinationUnitIntrospectorImpl() override;

  void BindToInterface(
      resource_coordinator::mojom::CoordinationUnitIntrospectorRequest request,
      const service_manager::BindSourceInfo& source_info);

  // Overridden from mojom::CoordinationUnitIntrospector:
  void GetProcessToURLMap(const GetProcessToURLMapCallback& callback) override;

  // Overridden from CoordinationUnitGraphObserver:
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;
  void OnWebContentsPropertyChanged(
      const WebContentsCoordinationUnitImpl* web_contents_cu,
      const mojom::PropertyType property_type,
      int64_t value) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitImpl* coordination_unit) override;

 private:
  struct WebContentsData {
    bool is_visible;
    base::TimeTicks last_invisible_time;
    base::TimeTicks navigation_finished_time;
  };
  // Note: |clock_| is always |&default_tick_clock_|, except during unit
  // testing.
  base::DefaultTickClock default_tick_clock_;
  base::TickClock* const clock_;
  mojo::BindingSet<mojom::CoordinationUnitIntrospector> bindings_;
  std::map<CoordinationUnitID, WebContentsData> web_contents_data_map_;
  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitIntrospectorImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_INTROSPECTOR_IMPL_H_
