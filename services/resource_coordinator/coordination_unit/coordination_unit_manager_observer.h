// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_MANAGER_OBSERVER_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_MANAGER_OBSERVER_H_

#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

namespace resource_coordinator {

class CoordinationUnitImpl;

// An observer API for the CoordinationUnitManager within GRC
//
// While the API does not enforce it (via aggressive usage of const)
// it is best practice to not mutate any CoordinationUnits
// TODO(matthalp, oysteine) should API use const arguments for CoordinationUnits
// This would require changes throughout the rest of the coordination_unit code
//
// To create a new observer:
//   (1) derive from this class
//   (2) register in // CoordinationUnitManager::RegisterObserver
//       inside of CoordinationUnitManager::CoordinationUnitManager
//       in coordination_unit_manager.cc
class CoordinationUnitManagerObserver {
 public:
  CoordinationUnitManagerObserver();
  virtual ~CoordinationUnitManagerObserver();

  // Called whenever any new CoordinationUnit of any type is created. Note that
  // this will be called after any specialized *CoordinationUnitCreated (e.g.
  // FrameCoordinationUnitCreation, etc.) is called
  virtual void CoordinationUnitCreated(
      CoordinationUnitImpl* coordination_unit) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitManagerObserver);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_MANAGER_OBSERVER_H_
