// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships_details.h"

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIP_TYPES_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIP_TYPES_H_

namespace resource_coordinator {

// Used to express an infinite cardinality for a relationship constraint.
constexpr size_t kInfinite = std::numeric_limits<size_t>::max();

// Types of relationships that CUs can have with each other.
enum class RelationshipType : size_t {
  kParent = 0,
  kChild,
  // Must be last.
  kMax,
};

// Returns the inverse relationship type.
RelationshipType Inverse(RelationshipType rel_type);

// Returns a string representation of the relationship type.
const char* ToString(RelationshipType rel_type);

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_H_
