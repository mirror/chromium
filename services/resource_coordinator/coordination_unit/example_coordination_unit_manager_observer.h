// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_EXAMPLE_COORDINATION_UNIT_MANAGER_OBSERVER_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_EXAMPLE_COORDINATION_UNIT_MANAGER_OBSERVER_H_

#include "base/macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_manager_observer.h"

namespace resource_coordinator {

class CoordinationUnitImpl;

class ExampleCoordinationUnitManagerObserver
    : public CoordinationUnitManagerObserver {
 public:
  ExampleCoordinationUnitManagerObserver();
  ~ExampleCoordinationUnitManagerObserver() override;

  void CoordinationUnitCreated(
      CoordinationUnitImpl* coordination_unit) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExampleCoordinationUnitManagerObserver);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_EXAMPLE_COORDINATION_UNIT_MANAGER_OBSERVER_H_
