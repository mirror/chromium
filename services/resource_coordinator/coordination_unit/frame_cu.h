// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_FRAME_CU_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_FRAME_CU_H_

#include "services/resource_coordinator/coordination_unit/coordination_unit_base_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships.h"

namespace resource_coordinator {

// Represents a Blink Frame, or equivalently, a browser content FrameTreeNode.
class FrameCU
    : public CoordinationUnitBaseImpl<FrameCU>,
      public HasExactlyOneParent<FrameCU, ProcessCU>,
      public HasAtMostOneParent<FrameCU, PageCU>,
      public HasAtMostOneParent<FrameCU, FrameCU>,
      public HasOptionalChildren<FrameCU, FrameCU> {
 public:
  FrameCU();
  ~FrameCU() override;

  // TODO: Add frame specific things here! Implement the frame mojo
  // interface!
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_FRAME_CU_H_
