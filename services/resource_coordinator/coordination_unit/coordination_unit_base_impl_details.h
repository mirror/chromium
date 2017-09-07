// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains template helpers for building a RelationshipsAreSatisfied
// (RelSat) function for CoordinationUnitBaseImpl. This is only meant to be
// included from coordination_unit_base_impl.h.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_IMPL_DETAILS_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_IMPL_DETAILS_H_

#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships.h"

namespace resource_coordinator {
namespace details {

// RelSatHelper forwards a call to ElementType::RelationshipsAreSatisfied if
// ElementType derives from
// details::HasRelationshipBase<kRelationshipType, kRelativeType>. Otherwise,
// it provides a dummy implementation which always replies true. This is
// intended to be used by RelSatRelationshipIterator and RelSatRelativeIterator
// in order to provide an implementation of
// CoordinationUnitBase::RelationshipsAreSatisfied.
template<class ElementType,
         RelationshipType kRelationshipType,
         CoordinationUnitType kRelativeType,
         bool kIsValidRelationship =
             std::is_base_of<
                ::resource_coordinator::details::HasRelationshipBase<
                    kRelationshipType, kRelativeType>,
                ElementType>::value>
struct RelSatHelper {};

// Specialization for when the relationship type is defined. Use
// the details::HasRelationshipBase::RelationshipsAreSatisfiedImpl function.
template<class ElementType,
         RelationshipType kRelationshipType,
         CoordinationUnitType kRelativeType>
struct RelSatHelper<ElementType, kRelationshipType, kRelativeType, true> {
  static bool RelSat(const ElementType* element) {
    auto* has_rel_base = static_cast<
        const ::resource_coordinator::details::HasRelationshipBase<
            kRelationshipType, kRelativeType>*>(element);
    return has_rel_base->RelationshipsAreSatisfiedImpl();
  }
};

// Specialization for when the relationship type is not defined. Simply
// return true, which is a nop when all of the results are combined using
// logical and.
template<class ElementType,
         RelationshipType kRelationshipType,
         CoordinationUnitType kRelativeType>
struct RelSatHelper<ElementType, kRelationshipType, kRelativeType, false> {
  static bool RelSat(const ElementType*) {
    return true;
  }
};

// Template for iterating over CoordinationUnitTypes and querying
// RelSatHelper::RelSat.
template<class ElementType,
         RelationshipType kRelationshipType,
         CoordinationUnitType kRelativeType>
struct RelSatRelativeIterator {
  static constexpr CoordinationUnitType kNextRelativeType =
      static_cast<CoordinationUnitType>(
          static_cast<size_t>(kRelativeType) + 1);
  static bool RelSat(const ElementType* element) {
    return RelSatHelper<ElementType, kRelationshipType,
                        kRelativeType>::RelSat(element) &&
        RelSatRelativeIterator<ElementType, kRelationshipType,
                               kNextRelativeType>::RelSat(element);
  }
};

// Specialization for the end of the iteration.
template<class ElementType, RelationshipType kRelationshipType>
struct RelSatRelativeIterator<ElementType, kRelationshipType,
                               CoordinationUnitType::kMax> {
  static bool RelSat(const ElementType*) { return true; }
};

// Template for iterating over RelationshipTypes and querying
// RelSatRelativeIterator::RelSat.
template<class ElementType, RelationshipType kRelationshipType>
struct RelSatRelationshipIterator {
  static constexpr RelationshipType kNextRelationshipType =
      static_cast<RelationshipType>(
          static_cast<size_t>(kRelationshipType) + 1);
  static constexpr CoordinationUnitType kFirstRelativeType =
      static_cast<CoordinationUnitType>(0u);
  static bool RelSat(const ElementType* element) {
    return RelSatRelativeIterator<ElementType, kRelationshipType,
                                  kFirstRelativeType>::RelSat(element) &&
        RelSatRelationshipIterator<ElementType,
                                   kNextRelationshipType>::RelSat(element);
  }
};

// Specialization for the end of the iteration.
template<class ElementType>
struct RelSatRelationshipIterator<ElementType, RelationshipType::kMax> {
  static bool RelSat(const ElementType*) {
    return true;
  }
};

// Syntactic sugar wrapper for RelSatRelationshipIterator.
template<class ElementType>
struct RelSatBuilder {
  static constexpr RelationshipType kFirstRelationshipType =
      static_cast<RelationshipType>(0u);
  static bool RelSat(const ElementType* element) {
    return RelSatRelationshipIterator<ElementType,
                                      kFirstRelationshipType>::RelSat(
        element);
  }
};

}  // namespace details
}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_IMPL_DETAILS_H_
