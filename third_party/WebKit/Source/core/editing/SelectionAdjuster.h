// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SelectionAdjuster_h
#define SelectionAdjuster_h

#include "core/CoreExport.h"
#include "core/editing/Forward.h"
#include "platform/wtf/Allocator.h"

namespace blink {

// |SelectionAdjuster| adjusts Selection to avoid crossing a boundary.
// Each boundary is defined by each function.
class CORE_EXPORT SelectionAdjuster final {
  STATIC_ONLY(SelectionAdjuster);

 public:
  static SelectionInDOMTree AdjustSelectionToAvoidCrossingShadowBoundaries(
      const SelectionInDOMTree&);
  static SelectionInFlatTree AdjustSelectionToAvoidCrossingShadowBoundaries(
      const SelectionInFlatTree&);
};

}  // namespace blink

#endif  // SelectionAdjuster_h
