// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_MANAGER_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_MANAGER_H_

#include <vector>

#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_manager_observer.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace resource_coordinator {

class CoordinationUnitManager {
 public:
  CoordinationUnitManager();
  ~CoordinationUnitManager();

  void OnStart(service_manager::BinderRegistry* registry,
               service_manager::ServiceContextRefFactory* service_ref_factory);
  void RegisterObserver(CoordinationUnitManagerObserver* observers);

  void NotifyObserversCoordinationUnitCreated(
      CoordinationUnitImpl* coordination_unit);
  void NotifyObserversCoordinationUnitWillBeDestroyed(
      CoordinationUnitImpl* coordination_unit);

 private:
  std::vector<CoordinationUnitManagerObserver*> observers_;

  static void Create(
      service_manager::ServiceContextRefFactory* service_ref_factory);

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitManager);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_MANAGER_H_
