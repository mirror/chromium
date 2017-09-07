// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PROCESS_CU_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PROCESS_CU_H_

#include "services/resource_coordinator/coordination_unit/coordination_unit_base_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships.h"

namespace resource_coordinator {

// Defines a Process Coordination Unit. There will be one of these in the graph
// per running Chrome process.
class ProcessCU
    : public CoordinationUnitBaseImpl<ProcessCU>,
      public HasOptionalChildren<ProcessCU, PageCU>,
      public HasOptionalChildren<ProcessCU, FrameCU> {
 public:
  ProcessCU();
  ~ProcessCU() override;

  // TODO: Add process specific things here! Implement the process mojo
  // interface!
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PROCESS_CU_H_
