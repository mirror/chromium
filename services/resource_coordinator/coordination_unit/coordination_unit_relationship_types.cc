// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_relationship_types.h"

#include "base/macros.h"

namespace resource_coordinator {

static const char kRelationshipTypeNames[] = { "Parent", "Child" };
static_assert(arraysize(kRelationshipTypeNames) ==
                  static_cast<size_t>(RelationshipType::kMax),
              "kRelationshipTypeNames out of date.");

RelationshipType Inverse(RelationshipType rel_type) {
  switch (rel_type) {
    case kParent: return kChild;
    case kChild: return kParent;
    case kMax: return kMax;
  }
  return kMax;
}

// Returns a string representation of the relationship type.
const char* ToString(RelationshipType rel_type) {
  DCHECK(rel_type != RelationshipType::kMax);
  return kRelationshipTypeNames[rel_type];
}

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_H_
