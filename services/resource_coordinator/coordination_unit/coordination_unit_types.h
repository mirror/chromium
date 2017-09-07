// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_TYPES_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_TYPES_H_

namespace resource_coordinator {

// This macro is where new coordination unit types are added. This macros is
// left in the global namespace for use externally.
#ifdef RESOURCE_COORDINATION_COORDINATION_UNIT_TYPE_ITERATOR
#error Macro name collision!
#endif
#define RESOURCE_COORDINATION_COORDINATION_UNIT_TYPE_ITERATOR(func)  \
    func(Process)  \
    func(Page)  \
    func(Frame)

// Generate the type enum.
#ifdef GRC_CU_ITERATOR_TYPE
#error Macro name collision!
#endif
#define GRC_CU_ITERATOR_TYPE(name)  k ## name,
enum class CoordinationUnitType : size_t {
  RESOURCE_COORDINATION_COORDINATION_UNIT_TYPE_ITERATOR(GRC_CU_ITERATOR_TYPE)
  // Must be last.
  kMax,
};
#undef GRC_CU_ITERATOR_TYPE

// Generate forward declarations.
#ifdef GRC_CU_ITERATOR_FDECL
#error Macro name collision!
#endif
#define GRC_CU_ITERATOR_FDECL(name)  class name ## CU;
RESOURCE_COORDINATION_COORDINATION_UNIT_TYPE_ITERATOR(GRC_CU_ITERATOR_FDECL)
#undef GRC_CU_ITERATOR_FDECL

// Generate a compile-time bidirectional mapping between CU-classes and the
// CU-type enum.
#ifdef GRC_CU_ITERATOR_CLASS_TYPE_MAP
#error Macro name collision!
#endif
#define GRC_CU_ITERATOR_CLASS_TYPE_MAP(name)  \
  template<> struct CoordinationUnitTypeFromClass<name ## CU> {  \
    static constexpr CoordinationUnitType kType =  \
        CoordinationUnitType::k ## name;  \
  };  \
  template<> struct CoordinationUnitClassFromType<  \
      CoordinationUnitType::k ## name> {  \
    using Type = name ## CU;  \
  };
template<class ElementType> struct CoordinationUnitTypeFromClass {};
template<CoordinationUnitType cu_type> struct CoordinationUnitClassFromType {};
RESOURCE_COORDINATION_COORDINATION_UNIT_TYPE_ITERATOR(GRC_CU_ITERATOR_CLASS_TYPE_MAP)
#undef DEFINE_CU_CLASS_TYPE_MAP

// Returns a string representation of the given coordination unit type.
const char* ToString(CoordinationUnitType cu_type);

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_TYPES_H_
