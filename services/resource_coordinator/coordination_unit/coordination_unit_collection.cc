// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_collection.h"

#include <algorithm>

namespace resource_coordinator {

CoordinationUnitCollection::CoordinationUnitCollection() = default;
CoordinationUnitCollection::~CoordinationUnitCollection() = default;

const std::vector<CoordinationUnitBase*>&
CoordinationUnitCollection::elements() const {
  return elements_;
}

std::vector<CoordinationUnitBase*> CoordinationUnitCollection::GetByType(
    CoordinationUnitType type) const {
  std::vector<CoordinationUnitBase*> elements;
  for (auto* element : elements_) {
    if (element->type() == type)
      elements.push_back(element);
  }
  return elements;
}

bool CoordinationUnitCollection::Has(CoordinationUnitBase* element) const {
  return std::find(elements_.begin(), elements_.end(), element) !=
      elements_.end();
}

bool CoordinationUnitCollection::Add(CoordinationUnitBase* element) {
  if (Has(element))
    return false;
  elements_.push_back(element);
  return true;
}

bool CoordinationUnitCollection::Remove(CoordinationUnitBase* element) {
  auto it = std::find(elements_.begin(), elements_.end(), element);
  if (it == elements_.end())
    return false;
  elements_.erase(it);
  return true;
}

}  // namespace resource_coordinator
