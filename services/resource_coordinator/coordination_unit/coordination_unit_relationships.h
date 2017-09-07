// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships_details.h"

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_H_

namespace resource_coordinator {

// Relationship helpers. These are classes from which coordination unit
// implementations derive from to express valid relationships between them.
// These will automatically endow the CU with the appropriate strongly typed
// accessors/mutators. New useful relationship types should be added here,
// and implemented in coordination_unit_relationships_details.h.

// Defines a relationship where instances of SelfType may have 0 to many
// ChildType children.
template<class SelfType, class ChildType> struct HasOptionalChildren;

// Defines a relationship where instances of SelfType may have 0 or 1 ChildType
// children.
template<class SelfType, class ChildType> struct HasAtMostOneChild;

// Defines a relationship where instances of SelfType must have exactly 1
// ChildType child.
template<class SelfType, class ChildType> struct HasExactlyOneChild;

// Defines a relationship where instances of SelfType must have exactly 1
// ParentType parent.
template<class SelfType, class ParentType> struct HasExactlyOneParent;

// Defines a relationship where instances of SelfType may have 0 or 1 ParentType
// parents.
template<class SelfType, class ParentType> struct HasAtMostOneParent;

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_RELATIONSHIPS_H_
