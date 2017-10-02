// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/process_host_signal_observer.h"

#include <utility>

#include "base/values.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace resource_coordinator {

const char kUserDataKey[] = "ProcessHostSignals";

class ProcessHostSignalsUserData : public base::SupportsUserData::Data {
 public:
  explicit ProcessHostSignalsUserData(CoordinationUnitImpl* coordination_unit,
                                      mojom::ProcessHostSignalsPtr request)
      : coordination_unit_(coordination_unit),
        process_host_signals_(std::move(request)) {
    DCHECK(process_host_signals_);

    bool should_use_background_priority = ShouldUseBackgroundPriority();
    process_host_signals_->ShouldHaveBackgroundPriority(
        should_use_background_priority);
    has_set_use_background_priority_ = should_use_background_priority;
  }

  ~ProcessHostSignalsUserData() override {}

  static ProcessHostSignalsUserData* Get(
      const CoordinationUnitImpl* coordination_unit) {
    if (coordination_unit->id().type == CoordinationUnitType::kProcess)
      return static_cast<ProcessHostSignalsUserData*>(
          coordination_unit->GetUserData(&kUserDataKey));

    std::set<CoordinationUnitImpl*> units =
        coordination_unit->GetAssociatedCoordinationUnitsOfType(
            CoordinationUnitType::kProcess);
    DCHECK_GE(1u, units.size());
    if (units.empty())
      return nullptr;

    ProcessHostSignalsUserData* user_data =
        static_cast<ProcessHostSignalsUserData*>(
            (*units.begin())->GetUserData(&kUserDataKey));
    DCHECK(user_data);

    return user_data;
  }

  void UpdateBackgroundPriority() {
    bool use_background_priority = ShouldUseBackgroundPriority();
    if (use_background_priority != has_set_use_background_priority_) {
      process_host_signals_->ShouldHaveBackgroundPriority(
          use_background_priority);
      has_set_use_background_priority_ = use_background_priority;
    }
  }

 private:
  bool ShouldUseBackgroundPriority() {
    for (auto* page_cu :
         coordination_unit_->GetAssociatedCoordinationUnitsOfType(
             CoordinationUnitType::kPage)) {
      int64_t visible;
      if (page_cu->GetProperty(mojom::PropertyType::kVisible, &visible)) {
        if (visible)
          return false;
      }
    }

    for (auto* frame_cu :
         coordination_unit_->GetAssociatedCoordinationUnitsOfType(
             CoordinationUnitType::kFrame)) {
      int64_t audible;
      if (frame_cu->GetProperty(mojom::PropertyType::kAudible, &audible)) {
        if (audible)
          return false;
      }
    }

    return true;
  }

  CoordinationUnitImpl* coordination_unit_;
  mojom::ProcessHostSignalsPtr process_host_signals_;
  bool has_set_use_background_priority_ = false;
};

ProcessHostSignalsObserver::ProcessHostSignalsObserver() = default;

ProcessHostSignalsObserver::~ProcessHostSignalsObserver() = default;

void ProcessHostSignalsObserver::AddProcessHostSignalObserver(
    mojom::ProcessHostSignalsPtr request,
    const CoordinationUnitID& id) {
  CoordinationUnitImpl* coordination_unit =
      CoordinationUnitImpl::GetCoordinationUnit(id);
  DCHECK(coordination_unit);
  coordination_unit->SetUserData(&kUserDataKey,
                                 base::MakeUnique<ProcessHostSignalsUserData>(
                                     coordination_unit, std::move(request)));

  ProcessHostSignalsUserData* user_data =
      static_cast<ProcessHostSignalsUserData*>(
          coordination_unit->GetUserData(&kUserDataKey));
  DCHECK(user_data);
  user_data->UpdateBackgroundPriority();
}

bool ProcessHostSignalsObserver::ShouldObserve(
    const CoordinationUnitImpl* coordination_unit) {
  auto coordination_unit_type = coordination_unit->id().type;
  return coordination_unit_type == CoordinationUnitType::kPage ||
         coordination_unit_type == CoordinationUnitType::kFrame ||
         coordination_unit_type == CoordinationUnitType::kProcess;
}

void ProcessHostSignalsObserver::OnFramePropertyChanged(
    const FrameCoordinationUnitImpl* frame_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  if (property_type != mojom::PropertyType::kAudible)
    return;

  ProcessHostSignalsUserData* user_data =
      ProcessHostSignalsUserData::Get(frame_cu);
  if (user_data)
    user_data->UpdateBackgroundPriority();
}

void ProcessHostSignalsObserver::OnPagePropertyChanged(
    const PageCoordinationUnitImpl* page_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  if (property_type != mojom::PropertyType::kVisible)
    return;

  ProcessHostSignalsUserData* user_data =
      ProcessHostSignalsUserData::Get(page_cu);
  if (user_data)
    user_data->UpdateBackgroundPriority();
}

void ProcessHostSignalsObserver::OnChildAdded(
    const CoordinationUnitImpl* coordination_unit,
    const CoordinationUnitImpl* child_coordination_unit) {
  if (coordination_unit->id().type != CoordinationUnitType::kProcess)
    return;

  ProcessHostSignalsUserData* user_data =
      ProcessHostSignalsUserData::Get(coordination_unit);
  if (user_data)
    user_data->UpdateBackgroundPriority();
}

void ProcessHostSignalsObserver::OnChildRemoved(
    const CoordinationUnitImpl* coordination_unit,
    const CoordinationUnitImpl* child_coordination_unit) {
  if (coordination_unit->id().type != CoordinationUnitType::kProcess)
    return;

  ProcessHostSignalsUserData* user_data =
      ProcessHostSignalsUserData::Get(coordination_unit);
  if (user_data)
    user_data->UpdateBackgroundPriority();
}

void ProcessHostSignalsObserver::BindToInterface(
    resource_coordinator::mojom::ProcessHostSignalsObserverRequest request,
    const service_manager::BindSourceInfo& source_info) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace resource_coordinator
