// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_VALIDATION_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_VALIDATION_H_

#include <set>

namespace resource_coordinator {

class CoordinationUnitBase;

// Validates the relationships that a CU has with its relatives are all
// reciprocal, that the CU relationship constraints are satisfied, and that
// it's relatives relationship constraints are also satisfied. This is useful as
// a local sanity check after a node is added to the CU graph; if the graph was
// valid before the new node was added, and IsLocallyValid returns true for the
// added none, then the graph remains globally valid as well.
bool IsLocallyValid(CoordinationUnitBase* cu);

// Validates that an entire graph is valid given one or more starting points.
// The graph may consist of disjoint subgraphs. This validates that
// each node satisfies its internal relationship constraints, and that all
// parent/child relationships are reciprocally valid.
bool IsGloballyValid(const std::set<CoordinationUnitBase*> roots);
bool IsGloballyValid(CoordinationUnitBase* root);

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_VALIDATION_H_
