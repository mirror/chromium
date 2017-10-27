// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/style/ComputedStyleInitialFunctions.h"

#include "core/style/FilterOperations.h"

namespace blink {

const FilterOperations& ComputedStyleInitialFunctions::InitialFilterInternal() {
  DEFINE_STATIC_LOCAL(FilterOperationsWrapper, ops,
                      (FilterOperationsWrapper::Create()));
  return ops.Operations();
}

}  // namespace blink
