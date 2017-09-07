// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <type_traits>

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_base_impl_details.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_relationships.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_types.h"

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_IMPL_H_

namespace resource_coordinator {

// Helper macros for generating member functions for modifying children and
// parents as necessary. These redirect to implementations provided by the
// appropriate HasRelationshipBase<> classes.
#if defined(GRC_DEFINE_CU_BASE_MEMBER_FUNCTION) || defined(GRC_DEFINE_CU_BASE_MEMBER_FUNCTION_HELPER)
#error Macro name collision!
#endif
#define GRC_DEFINE_CU_BASE_MEMBER_FUNCTION_HELPER(  \
    base, action, relation, rettype, argtype, argname)  \
  template<typename RelativeType,  \
           typename = typename std::enable_if<  \
               std::is_base_of<  \
                   ::resource_coordinator::details::base<  \
                      RelationshipType::k ## relation,  \
                      CoordinationUnitTypeFromClass<RelativeType>::kType>,  \
                   SelfType>::value>::type>  \
  rettype action ## relation(argtype argname) {  \
    return static_cast<  \
        ::resource_coordinator::details::base<  \
            RelationshipType::k ## relation,  \
            CoordinationUnitTypeFromClass<RelativeType>::kType>*>(  \
        static_cast<SelfType*>(this))->action ## Impl(argname);  \
  }
#define GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(base, action, rettype, argtype,  \
                                       argname)  \
    GRC_DEFINE_CU_BASE_MEMBER_FUNCTION_HELPER(  \
        base, action, Parent, rettype, argtype, argname)  \
    GRC_DEFINE_CU_BASE_MEMBER_FUNCTION_HELPER(  \
        base, action, Child, rettype, argtype, argname)

// Base class for all coordination units. Implements pure virtual portions of
// CoordinationUnitBase. Uses template magic to generate strongly typed
// (Add|Clear|Get|Has|Remove)(Child|Parent) functions to this class.
template<class SelfType>
class CoordinationUnitBaseImpl : public CoordinationUnitBase {
 public:
  static constexpr CoordinationUnitType kType =
      CoordinationUnitTypeFromClass<SelfType>::kType;

  ~CoordinationUnitBaseImpl() override {}

  // Implementation of CoordinationUnitBase::type:
  CoordinationUnitType type() const override {
    return kType;
  }

  // Implementation of CoordinationUnitBase::RelationshipsAreSatisfied:
  bool RelationshipsAreSatisfied() const override {
    // This expands to calling all of the
    // details::HasRelationshipBase::RelationshipsAreSatisfied functions for
    // relationships that this CU actually has defined.
    return ::resource_coordinator::details::RelSatBuilder<SelfType>::RelSat(
        static_cast<const SelfType*>(this));
  }

  // Define various accessors as necessary. These will only compile if the
  // relationship actually exists and has been expressed in the CU-class
  // definition (using SFINAE). These all be defined in the same name scope
  // (class) because name lookup and overload resolution are performed in
  // separate phases; had these been defined in in each of the HasRelationship
  // classes there would be ambiguity and compilation would fail.
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasRelationshipBase, Add, bool, RelativeType*, relative)
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasRelationshipBase, Remove, bool, RelativeType*, relative)
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasRelationshipBase, Has, bool, RelativeType*, relative)
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasRelationshipBase, Get, bool, std::vector<RelativeType*>*, relatives)

  // Specialized accessors for relationships with a max cardinality of 1.
  // These go through the definitions in HasSingleRelationshipBase.
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasSingleRelationshipBase, Set, bool, RelativeType*, relative)
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasSingleRelationshipBase, Clear, bool, , )
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasSingleRelationshipBase, Has, bool, , )
  GRC_DEFINE_CU_BASE_MEMBER_FUNCTION(
      HasSingleRelationshipBase, Get, RelativeType*, , )
};

#undef GRC_DEFINE_CU_BASE_MEMBER_FUNCTION_HELPER
#undef GRC_DEFINE_CU_BASE_MEMBER_FUNCTION

// Links a parent and a child, by setting reciprocal relationships on both of
// them.
template<class ParentType, class ChildType>
void LinkParentChild(ParentType* parent, ChildType* child) {
  parent->AddChild(child);
  child->AddParent(parent);
}

// Unlinks a parent and a child by removing reciprocal relationships from both
// of them.
template<class ParentType, class ChildType>
void UnlinkParentChild(ParentType* parent, ChildType* child) {
  parent->RemoveChild(child);
  child->RemoveParent(parent);
}

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_IMPL_H_
