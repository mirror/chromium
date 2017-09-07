// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_

#include <cstdint>

#include "services/resource_coordinator/coordination_unit/coordination_unit_collection.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_types.h"

namespace resource_coordinator {

// The concrete base class which backs all CU classes. This allows full
// introspection of a CU and its relationships. A few methods are virtual and
// are implemented by the templated CoordinationUnitBaseImpl, which is the
// actual class from which CU classes derive.
class CoordinationUnitBase {
 public:
  CoordinationUnitBase();
  virtual ~CoordinationUnitBase();

  // Returns the type of this coordination unit. This is implemented by
  // CoordinationUnitBaseImpl.
  virtual CoordinationUnitType type() const = 0;

  // Returns true iff the relationship constraints on this particular CU are
  // satisfied. This does not mean that its relationships are mirrored
  // appropriately by its relatives, nor that the entire graph is valid. This is
  // implemented by CoordinationUnitBaseImpl.
  virtual bool RelationshipsAreSatisfied() const = 0;

  // The typename of the CU.
  const char* TypeName() const;

  // A guaranteed unique ID for the lifetime of a CU. Useful for tracing, etc.
  // Effectively just the value of "this".
  uintptr_t Id() const;

  // Accessors for the collection of parents.
  CoordinationUnitCollection& parents();
  const CoordinationUnitCollection& parents() const;

  // Accessors for the collection of children.
  CoordinationUnitCollection& children();
  const CoordinationUnitCollection& children() const;

  // Accessors for the relatives, keyed by type. Enables more generic runtime
  // introspection.
  CoordinationUnitCollection& relatives(
      RelationshipType relationship_type);
  const CoordinationUnitCollection& relatives(
      RelationshipType relationship_type) const;

  // Functions with the following signatures and types are created by
  // CoordinationUnitBaseImpl for each "relationship" that is defined for a CU.

  // Adds a parent|child to the CU. Returns true if it as added, false
  // otherwise. This can refuce to add a parent if the cardinality of the
  // relationship is exhausted.
  // bool Add(Parent|Child)(RelativeType* relative)

  // Removes a parent|child from the CU. Returns true if it existed and was
  // removed, false if it didn't exist.
  // bool Remove(Parent|Child)(RelativeType* relative)

  // Returns true if the given |relative| is a parent|child of this CU.
  // bool Has(Parent|Child)(RelativeType* relative)

  // Gets all of the parent|children of the given type from this CU, appending
  // them to the provided |relatives| vector. Returns the number of relatives
  // fetched. |relatives| may be nullptr.
  // size_t Get(Parent|Child)(std::vector<RelativeType*>* relatives)

  // Functions of the following signatures are added for relationships with a
  // maximum cardinality of 1.

  // Sets the parent|child on this CU. If a parent or child already existed this
  // will overwrite it. Returns true if a value was overwritten, false
  // otherwise. |relative| may be nullptr, in which case this is the same as
  // calling Clear(Parent|Child).
  // bool Set(Parent|Child)(RelativeType* relative);

  // Clears the parent|child from this CU. Returns true if there was something
  // to clear, false if it was already clear.
  // bool Clear(Parent|Child)();

  // Returns true if a parent|child is set on this CU, false otherwise.
  // bool HasOne(Parent|Child)();

  // Gets the parent|child from this CU. Returns nullptr if there is none.
  // RelativeType Get(Parent|Child)();

 private:
  CoordinationUnitCollection relatives_[
      static_cast<size_t>(RelationshipType::kMax)];
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_
