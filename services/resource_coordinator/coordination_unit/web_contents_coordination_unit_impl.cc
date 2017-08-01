// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"

#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"

namespace resource_coordinator {

WebContentsCoordinationUnitImpl::WebContentsCoordinationUnitImpl(
    const CoordinationUnitID& id,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : CoordinationUnitImpl(id, std::move(service_ref)) {}

WebContentsCoordinationUnitImpl::~WebContentsCoordinationUnitImpl() = default;

std::set<CoordinationUnitImpl*>
WebContentsCoordinationUnitImpl::GetAssociatedCoordinationUnitsOfType(
    CoordinationUnitType type) {
  switch (type) {
    case CoordinationUnitType::kProcess: {
      std::set<CoordinationUnitImpl*> process_coordination_units;

      // There is currently not a direct relationship between processes and
      // tabs. However, frames are children of both processes and frames, so we
      // find all of the processes that are reachable from the tabs's child
      // frames.
      for (auto* frame_coordination_unit :
           GetChildCoordinationUnitsOfType(CoordinationUnitType::kFrame)) {
        for (auto* process_coordination_unit :
             frame_coordination_unit->GetAssociatedCoordinationUnitsOfType(
                 CoordinationUnitType::kProcess)) {
          process_coordination_units.insert(process_coordination_unit);
        }
      }

      return process_coordination_units;
    }
    case CoordinationUnitType::kFrame:
      return GetChildCoordinationUnitsOfType(type);
    default:
      return std::set<CoordinationUnitImpl*>();
  }
}

double WebContentsCoordinationUnitImpl::CalculateCPUUsage() {
  double cpu_usage = 0.0;

  for (auto* process_coordination_unit :
       GetAssociatedCoordinationUnitsOfType(CoordinationUnitType::kProcess)) {
    size_t tabs_in_process = process_coordination_unit
                                 ->GetAssociatedCoordinationUnitsOfType(
                                     CoordinationUnitType::kWebContents)
                                 .size();
    DCHECK_LE(1u, tabs_in_process);

    base::Value process_cpu_usage_value =
        process_coordination_unit->GetProperty(mojom::PropertyType::kCPUUsage);
    double process_cpu_usage =
        process_cpu_usage_value.IsType(base::Value::Type::NONE)
            ? 0.0
            : process_cpu_usage_value.GetDouble();
    cpu_usage += process_cpu_usage / tabs_in_process;
  }

  return cpu_usage;
}

bool WebContentsCoordinationUnitImpl::CalculateExpectedTaskQueueingDuration(
    double* output) {
  // Calculate the EQT for the process of the main frame only because
  // the smoothness of the main frame may affect the users the most.
  CoordinationUnitImpl* main_frame_cu = GetMainFrameCoordinationUnit();
  if (!main_frame_cu)
    return false;

  auto associated_processes =
      main_frame_cu->GetAssociatedCoordinationUnitsOfType(
          CoordinationUnitType::kProcess);

  size_t num_processes_per_frame = associated_processes.size();
  // A frame should not belong to more than 1 process.
  DCHECK_LE(num_processes_per_frame, 1u);

  if (num_processes_per_frame == 0)
    return false;

  base::Value process_eqt_value =
      (*associated_processes.begin())
          ->GetProperty(mojom::PropertyType::kExpectedTaskQueueingDuration);
  DCHECK(process_eqt_value.is_double());
  *output = process_eqt_value.GetDouble();
  return true;
}

void WebContentsCoordinationUnitImpl::RecalculateProperty(
    const mojom::PropertyType property_type) {
  switch (property_type) {
    case mojom::PropertyType::kCPUUsage: {
      SetProperty(mojom::PropertyType::kCPUUsage,
                  base::MakeUnique<base::Value>(CalculateCPUUsage()));
      break;
    }
    case mojom::PropertyType::kExpectedTaskQueueingDuration: {
      double eqt;
      if (CalculateExpectedTaskQueueingDuration(&eqt))
        SetProperty(property_type, base::MakeUnique<base::Value>(eqt));
      break;
    }
    default:
      NOTREACHED();
  }
}

CoordinationUnitImpl*
WebContentsCoordinationUnitImpl::GetMainFrameCoordinationUnit() {
  for (auto* cu :
       GetAssociatedCoordinationUnitsOfType(CoordinationUnitType::kFrame)) {
    if (ToFrameCoordinationUnit(cu)->IsMainFrame())
      return cu;
  }

  return nullptr;
}

}  // namespace resource_coordinator
