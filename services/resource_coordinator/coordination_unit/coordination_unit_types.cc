// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_types.h"

#include "base/macros.h"

namespace resource_coordinator {

// Generate string names for each of the CU types.
#if defined(GRC_STRINGIFY) || defined(GRC_TOSTRING) || defined (GRC_CU_ITERATOR_NAME)
#error Macro name collision!
#endif
#define GRC_STRINGIFY(x) #x
#define GRC_TOSTRING(x) STRINGIFY(x)
#define GRC_CU_ITERATOR_NAME(name) GRC_TOSTRING(name)
static const char* kCoordinationUnitTypeNames[] = {
  RESOURCE_COORDINATION_COORDINATION_UNIT_TYPE_ITERATOR(GRC_CU_ITERATOR_NAME)
};
#undef GRC_CU_ITERATOR_NAME
#undef GRC_TOSTRING
#undef GRC_STRINGIFY

// Sanity check that the enum and the string table are the same size.
static_assert(arraysize(kCoordinationUnitTypeNames) ==
                  static_cast<size_t>(CoordinationUnitType::kMax),
              "kCoordinationUnitTypeNames out of date.");

const char* ToString(CoordinationUnitType cu_type) {
  DCHECK(cu_type != CoordinationUnitType::kMax);
  return kCoordinationUnitTypeNames[static_cast<size_t>(cu_type)];
}

}  // namespace resource_coordinator
