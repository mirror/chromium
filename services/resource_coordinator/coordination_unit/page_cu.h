// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_CU_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_CU_H_

#include "services/resource_coordinator/coordination_unit/coordination_unit_base_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships.h"

namespace resource_coordinator {

// Defines a Page Coordination Unit. This is logically equivalen to a Blink
// Page objet, or a tab in a browser window. It represents a running web page
// with user visible surface.
class PageCU
    : public CoordinationUnitBaseImpl<PageCU>,
      public HasExactlyOneParent<PageCU, ProcessCU>,
      public HasExactlyOneChild<PageCU, FrameCU> {
 public:
  PageCU();
  ~PageCU() override;

  // TODO: Add page specific things here! Implement the page mojo
  // interface!
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_CU_H_
