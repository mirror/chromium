// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_manager.h"

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_manager_observer.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_provider_impl.h"
#include "services/resource_coordinator/coordination_unit/example_coordination_unit_manager_observer.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace resource_coordinator {

class CoordinationUnitImpl;

CoordinationUnitManager::CoordinationUnitManager() {
  RegisterObserver(new ExampleCoordinationUnitManagerObserver());
}

CoordinationUnitManager::~CoordinationUnitManager() {
  for (CoordinationUnitManagerObserver* observer : observers_) {
    delete observer;
  }
}

void CoordinationUnitManager::OnStart(
    service_manager::BinderRegistry* registry,
    service_manager::ServiceContextRefFactory* service_ref_factory) {
  registry->AddInterface(base::Bind(&CoordinationUnitProviderImpl::Create,
                                    base::Unretained(service_ref_factory),
                                    base::Unretained(this)));
}

void CoordinationUnitManager::RegisterObserver(
    CoordinationUnitManagerObserver* observer) {
  observers_.push_back(observer);
}

void CoordinationUnitManager::NotifyObserversCoordinationUnitCreated(
    CoordinationUnitImpl* coordination_unit) {
  for (CoordinationUnitManagerObserver* observer : observers_) {
    observer->CoordinationUnitCreated(coordination_unit);
  }
}

void CoordinationUnitManager::NotifyObserversCoordinationUnitWillBeDestroyed(
    CoordinationUnitImpl* coordination_unit) {
  coordination_unit->WillBeDestroyed();
}

}  // namespace resource_coordinator
