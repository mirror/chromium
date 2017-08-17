// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"

#include "base/logging.h"
#include "base/values.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"

namespace resource_coordinator {

ProcessCoordinationUnitImpl::ProcessCoordinationUnitImpl(
    const CoordinationUnitID& id,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : CoordinationUnitImpl(id, std::move(service_ref)) {}

ProcessCoordinationUnitImpl::~ProcessCoordinationUnitImpl() = default;

std::set<CoordinationUnitImpl*>
ProcessCoordinationUnitImpl::GetAssociatedCoordinationUnitsOfType(
    CoordinationUnitType type) const {
  switch (type) {
    case CoordinationUnitType::kWebContents: {
      // There is currently not a direct relationship between processes and
      // tabs. However, frames are children of both processes and frames, so we
      // find all of the tabs that are reachable from the process's child
      // frames.
      std::set<CoordinationUnitImpl*> web_contents_coordination_units;

      for (auto* frame_coordination_unit :
           GetChildCoordinationUnitsOfType(CoordinationUnitType::kFrame)) {
        for (auto* web_contents_coordination_unit :
             frame_coordination_unit->GetAssociatedCoordinationUnitsOfType(
                 CoordinationUnitType::kWebContents)) {
          web_contents_coordination_units.insert(
              web_contents_coordination_unit);
        }
      }

      return web_contents_coordination_units;
    }
    case CoordinationUnitType::kFrame:
      return GetChildCoordinationUnitsOfType(type);
    default:
      return std::set<CoordinationUnitImpl*>();
  }
}

void ProcessCoordinationUnitImpl::PropagateProperty(
    mojom::PropertyType property_type,
    int64_t value) {
  switch (property_type) {
    // Trigger tab coordination units to recalculate their CPU usage.
    case mojom::PropertyType::kCPUUsage: {
      for (auto* tab_coordination_unit : GetAssociatedCoordinationUnitsOfType(
               CoordinationUnitType::kWebContents)) {
        tab_coordination_unit->RecalculateProperty(
            mojom::PropertyType::kCPUUsage);
      }
      break;
    }
    case mojom::PropertyType::kExpectedTaskQueueingDuration: {
      // Do not propagate if the associated frame is not the main frame.
      for (auto* frame_cu :
           GetAssociatedCoordinationUnitsOfType(CoordinationUnitType::kFrame)) {
        if (!ToFrameCoordinationUnit(frame_cu)->IsMainFrame())
          continue;

        auto associated_tabs = frame_cu->GetAssociatedCoordinationUnitsOfType(
            CoordinationUnitType::kWebContents);

        size_t num_tabs_per_frame = associated_tabs.size();

        // A frame should not belong to more than 1 tab.
        DCHECK_LE(1u, num_tabs_per_frame);

        // Can be associated with no tab before the navigation finishes.
        if (num_tabs_per_frame == 0)
          continue;

        (*associated_tabs.begin())
            ->RecalculateProperty(
                mojom::PropertyType::kExpectedTaskQueueingDuration);
      }
      break;
    }
    default:
      break;
  }
}

}  // namespace resource_coordinator
