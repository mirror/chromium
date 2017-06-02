// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/example_coordination_unit_manager_observer.h"

#include <stdint.h>

#include <ostream>

#include "base/values.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

namespace resource_coordinator {

namespace {

std::ostream& operator<<(std::ostream& os, const CoordinationUnitID& cu_id) {
  os << "(" << static_cast<uint64_t>(cu_id.type) << ", "
     << static_cast<uint64_t>(cu_id.id) << ")";

  return os;
}

}  // namespace

ExampleCoordinationUnitManagerObserver::
    ExampleCoordinationUnitManagerObserver() = default;

ExampleCoordinationUnitManagerObserver::
    ~ExampleCoordinationUnitManagerObserver() = default;

void ExampleCoordinationUnitManagerObserver::CoordinationUnitCreated(
    CoordinationUnitImpl* coordination_unit) {
  LOG(ERROR) << "Created " << coordination_unit->id();

  coordination_unit->add_child_event_listeners().AddListener(
      base::Bind([](CoordinationUnitImpl* cu, CoordinationUnitImpl* child_cu) {
        LOG(ERROR) << "AddChild " << cu->id() << " <--- " << child_cu->id();
      }));

  coordination_unit->add_parent_event_listeners().AddListener(
      base::Bind([](CoordinationUnitImpl* cu, CoordinationUnitImpl* parent_cu) {
        LOG(ERROR) << "AddParent " << cu->id() << " ---> " << parent_cu->id();
      }));

  coordination_unit->remove_child_event_listeners().AddListener(
      base::Bind([](CoordinationUnitImpl* cu, CoordinationUnitImpl* child_cu) {
        LOG(ERROR) << "RemoveChild " << cu->id() << " <-/- " << child_cu->id();
      }));

  coordination_unit->remove_parent_event_listeners().AddListener(
      base::Bind([](CoordinationUnitImpl* cu, CoordinationUnitImpl* parent_cu) {
        LOG(ERROR) << "RemoveParent " << cu->id() << " -/-> "
                   << parent_cu->id();
      }));

  coordination_unit->storage_property_changed_event_listeners().AddListener(
      base::Bind([](CoordinationUnitImpl* cu, mojom::PropertyType property) {
        LOG(ERROR) << "PropertyChanged " << cu->id() << " " << property
                   << " <= " << cu->GetProperty(property);
      }));

  coordination_unit->SetProperty(mojom::PropertyType::kTest, base::Value(37));

  coordination_unit->will_be_destroyed_event_listeners().AddListener(
      base::Bind([](CoordinationUnitImpl* cu) {
        LOG(ERROR) << "Destroyed " << cu->id();
      }));
}

}  // namespace resource_coordinator
