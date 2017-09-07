// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"

namespace resource_coordinator {

CoordinationUnitBase::CoordinationUnitBase() = default;
CoordinationUnitBase::~CoordinationUnitBase() = default;

CoordinationUnitType CoordinationUnitBase::type() const = 0;
virtual bool CoordinationUnitBase::RelationshipsAreSatisfied() const = 0;

// The typename of the CU.
const char* TypeName() const {
  return kCoordinationUnitTypeNames[static_cast<size_t>(type())];
}

// A guaranteed unique ID for a CU.
uintptr_t Id() const {
  return reinterpret_cast<uintptr_t>(this);
}

CoordinationUnitCollection& parents() {
  return relatives_[static_cast<size_t>(RelationshipType::kParent)];
}
const CoordinationUnitCollection& parents() const {
  return relatives_[static_cast<size_t>(RelationshipType::kParent)];
}

CoordinationUnitCollection& children() {
  return relatives_[static_cast<size_t>(RelationshipType::kChild)];
}
const CoordinationUnitCollection& children() const {
  return relatives_[static_cast<size_t>(RelationshipType::kChild)];
}

CoordinationUnitCollection& relatives(
    RelationshipType relationship_type) {
  return relatives_[static_cast<size_t>(relationship_type)];
}
const CoordinationUnitCollection& relatives(
    RelationshipType relationship_type) const {
  return relatives_[static_cast<size_t>(relationship_type)];
}

}  // namespace resource_coordinator
