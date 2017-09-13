// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_introspector_impl.h"

#include <vector>

#include "base/process/process_handle.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/web_contents_coordination_unit_impl.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace resource_coordinator {

CoordinationUnitIntrospectorImpl::CoordinationUnitIntrospectorImpl()
    : clock_(&default_tick_clock_) {}

CoordinationUnitIntrospectorImpl::~CoordinationUnitIntrospectorImpl() = default;

void CoordinationUnitIntrospectorImpl::GetProcessToURLMap(
    const GetProcessToURLMapCallback& callback) {
  std::vector<resource_coordinator::mojom::ProcessInfoPtr> process_infos;
  std::vector<CoordinationUnitImpl*> process_cus =
      CoordinationUnitImpl::GetCoordinationUnitsOfType(
          CoordinationUnitType::kProcess);
  for (CoordinationUnitImpl* process_cu : process_cus) {
    int64_t pid;
    if (!process_cu->GetProperty(mojom::PropertyType::kPID, &pid))
      continue;

    mojom::ProcessInfoPtr process_info(mojom::ProcessInfo::New());
    process_info->pid = pid;
    DCHECK_NE(base::kNullProcessId, process_info->pid);

    std::set<CoordinationUnitImpl*> web_contents_cus =
        process_cu->GetAssociatedCoordinationUnitsOfType(
            CoordinationUnitType::kWebContents);
    for (CoordinationUnitImpl* web_contents_cu : web_contents_cus) {
      int64_t ukm_source_id;
      if (web_contents_cu->GetProperty(
              resource_coordinator::mojom::PropertyType::kUKMSourceId,
              &ukm_source_id)) {
        process_info->ukm_source_ids.push_back(ukm_source_id);
      }
      if (web_contents_data_map_[web_contents_cu->id()].is_visible) {
        process_info->time_since_last_visible.push_back(-1);
      } else {
        auto now = clock_->NowTicks();
        auto duration =
            now -
            web_contents_data_map_[web_contents_cu->id()].last_invisible_time;
        process_info->time_since_last_visible.push_back(duration.InSeconds());
      }
    }
    process_infos.push_back(std::move(process_info));
  }
  callback.Run(std::move(process_infos));
}

void CoordinationUnitIntrospectorImpl::BindToInterface(
    resource_coordinator::mojom::CoordinationUnitIntrospectorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  bindings_.AddBinding(this, std::move(request));
}

bool CoordinationUnitIntrospectorImpl::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  return coordination_unit->id().type == CoordinationUnitType::kWebContents;
}

void CoordinationUnitIntrospectorImpl::OnWebContentsPropertyChanged(
    const WebContentsCoordinationUnitImpl* web_contents_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  const auto web_contents_cu_id = web_contents_cu->id();
  if (property_type == mojom::PropertyType::kVisible) {
    if (value) {
      // The web contents becomes visible again, clear all record in order to
      // report metrics when web contents becomes invisible next time.
      web_contents_data_map_[web_contents_cu_id].is_visible = true;
      return;
    }
    web_contents_data_map_[web_contents_cu_id].is_visible = false;
    web_contents_data_map_[web_contents_cu_id].last_invisible_time =
        clock_->NowTicks();
  }
}

void CoordinationUnitIntrospectorImpl::OnBeforeCoordinationUnitDestroyed(
    const CoordinationUnitImpl* coordination_unit) {
  if (coordination_unit->id().type == CoordinationUnitType::kWebContents) {
    web_contents_data_map_.erase(coordination_unit->id());
  }
}

}  // namespace resource_coordinator
