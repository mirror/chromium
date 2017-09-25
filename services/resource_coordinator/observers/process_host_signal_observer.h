// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_OBSERVERS_PROCESS_HOST_SIGNAL_OBSERVER_H_
#define SERVICES_RESOURCE_COORDINATOR_OBSERVERS_PROCESS_HOST_SIGNAL_OBSERVER_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/interfaces/process_host_signals.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}  // namespace service_manager

namespace resource_coordinator {

class CoordinationUnitImpl;
class FrameCoordinationUnitImpl;
class PageCoordinationUnitImpl;

class ProcessHostSignalsObserver : public CoordinationUnitGraphObserver,
                                   public mojom::ProcessHostSignalsObserver {
 public:
  ProcessHostSignalsObserver();
  ~ProcessHostSignalsObserver() override;

  // mojom::ProcessHostSignalsObserver implementation.
  void AddProcessHostSignalObserver(mojom::ProcessHostSignalsPtr observer,
                                    const CoordinationUnitID& id) override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override;
  void OnFramePropertyChanged(const FrameCoordinationUnitImpl* frame_cu,
                              const mojom::PropertyType property_type,
                              int64_t value) override;
  void OnPagePropertyChanged(const PageCoordinationUnitImpl* page_cu,
                             const mojom::PropertyType property_type,
                             int64_t value) override;

  void OnChildAdded(
      const CoordinationUnitImpl* coordination_unit,
      const CoordinationUnitImpl* child_coordination_unit) override;

  void OnChildRemoved(
      const CoordinationUnitImpl* coordination_unit,
      const CoordinationUnitImpl* child_coordination_unit) override;

  void BindToInterface(
      resource_coordinator::mojom::ProcessHostSignalsObserverRequest request,
      const service_manager::BindSourceInfo& source_info);

 private:
  mojo::BindingSet<mojom::ProcessHostSignalsObserver> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ProcessHostSignalsObserver);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_OBSERVERS_PROCESS_HOST_SIGNAL_OBSERVER_H_
