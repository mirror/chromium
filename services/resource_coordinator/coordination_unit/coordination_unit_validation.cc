// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_validation.h"

namespace resource_coordinator {

bool IsLocallyValid(CoordinationUnitBase* cu) {
  if (!cu->RelationshipsAreSatisfied())
    return false;

  std::set<CoordinationUnitBase*> relatives;

  // Visit all children. Ensure that they have an appropriate parent
  // relationship with this node. Schedule them to be validated as well.
  for (auto* child : cu->children().elements()) {
    if (!child->parents().Has(cu))
      return false;
    relatives.insert(child);
  }

  // Ditto with the parent.
  for (auto* parent : cu->parents().elements()) {
    if (!parent->children().Has(cu))
      return false;
    relatives.insert(parent);
  }

  // Now validate all of the relatives relationship constraints.
  for (auto* relative : relatives) {
    if (!relative->RelationshipsAreSatisfied())
      return false;
  }

  return true;
}

bool IsGloballyValid(const std::set<CoordinationUnitBase*> roots) {
  std::vector<CoordinationUnitBase*> to_visit(
      roots.begin(), roots.end());
  std::set<CoordinationUnitBase*> visited;
  while (!to_visit.empty()) {
    auto* current = to_visit.back();
    to_visit.pop_back();

    // Skip already visited elements.
    if (!visited.insert(current).second)
      continue;

    // Check that the current node is valid.
    if (!current->RelationshipsAreSatisfied())
      return false;

    // Check the children have reciprocal relationships.
    for (auto* child : current->children().elements()) {
      if (!child->parents().Has(current))
        return false;
      // Schedule unvisited children for further inspection.
      if (visited.find(child) == visited.end())
        to_visit.push_back(child);
    }

    // Ditto for the parents.
    for (auto* parent : current->parents().elements()) {
      if (!parent->children().Has(current))
        return false;
      // Schedule unvisited parents for further inspection.
      if (visited.find(parent) == visited.end())
        to_visit.push_back(parent);
    }
  }

  return true;
}

bool IsGloballyValid(CoordinationUnitBase* root) {
  std::set<CoordinationUnitBase*> roots;
  roots.insert(root);
  return IsGloballyValid(roots);
}

}  // namespace resource_coordinator
