// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGReplacedSize_h
#define NGReplacedSize_h

#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_physical_size.h"
#include "core/layout/ng/ng_min_max_content_size.h"
#include "platform/LayoutUnit.h"
#include "platform/wtf/Optional.h"

namespace blink {

class ComputedStyle;
class NGConstraintSpace;

// Data and routines for computing size of a replaced element.
// Legacy implementation is at
// LayoutReplaced::ComputeReplacedLogicalWidth/Height.
struct CORE_EXPORT NGReplacedSize {
  // Returns border-box size width.
  LayoutUnit ComputeReplacedWidth(
      const NGConstraintSpace& space,
      const ComputedStyle& style,
      const Optional<MinMaxContentSize>& child_minmax) const;

  // Returns border-box size height.
  LayoutUnit ComputeReplacedHeight(
      const NGConstraintSpace& space,
      const ComputedStyle& style,
      const Optional<MinMaxContentSize>& child_minmax) const;

  NGPhysicalSize intrinsic_content_size;  // Default intrinsic size.
  // Fields below are derived from LayoutReplaced::IntrinsicSizingInfo.
  Optional<LayoutUnit> content_width;   // Computed intrinsic width.
  Optional<LayoutUnit> content_height;  // Computed intrinsic height.
  Optional<NGPhysicalSize> aspect_ratio;
};

}  // namespace blink

#endif  // NGReplacedSize_h
