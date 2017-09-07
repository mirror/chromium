// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_COLLECTION_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_COLLECTION_H_

#include <vector>

#include "base/macros.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_types.h"

namespace resource_coordinator {

class CoordinationUnitBase;
namespace details {
class CoordinationUnitCollectionWriter;
}

// Represents a collection of coordination units. Each coordination unit
// contains two of these, one for housing parents and another for housing
// children.
class CoordinationUnitCollection {
 public:
  CoordinationUnitCollection();
  ~CoordinationUnitCollection();

  // Direct access to the elements in this collection.
  const std::vector<CoordinationUnitBase*>& elements() const;

  // Returns all of the elements in this collection of a given type.
  std::vector<CoordinationUnitBase*> GetByType(
      CoordinationUnitType type) const;

  // Returns true if the given element is in this collection, false otherwise.
  bool Has(CoordinationUnitBase* element) const;

 protected:
  // This allows internals to have write access to collections.
  friend class details::CoordinationUnitCollectionWriter;

  // Write access is limited via the Writer friend class. Users of CU objects
  // should use the strongly typed (Add|Set|Remove|Has)(Child|Parent) functions.

  // Adds a CU to this collection. Returns true if the element was added, false
  // if it already existed. In both cases the element is guaranteed to exist in
  // the collection after return.
  bool Add(CoordinationUnitBase* element);

  // Removes a CU from this collection. Returns true if the element was found,
  // false otherwise.
  bool Remove(CoordinationUnitBase* element);

 private:
  std::vector<CoordinationUnitBase*> elements_;

 DISALLOW_COPY_AND_ASSIGN(CoordinationUnitCollection);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_COLLECTION_H_
