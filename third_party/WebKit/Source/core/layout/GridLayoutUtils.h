// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GridLayoutUtils_h
#define GridLayoutUtils_h

#include "core/layout/LayoutBox.h"
#include "core/style/GridPositionsResolver.h"
#include "platform/LayoutUnit.h"

namespace blink {

enum GridAxis { kGridRowAxis, kGridColumnAxis };

class LayoutGrid;

class GridLayoutUtils {
 public:
  static LayoutUnit MarginLogicalWidthForChild(const LayoutGrid&,
                                               const LayoutBox&);
  static LayoutUnit MarginLogicalHeightForChild(const LayoutGrid&,
                                                const LayoutBox&);
  static LayoutUnit MarginOverForChild(const LayoutGrid&,
                                       const LayoutBox&,
                                       GridAxis);
  static LayoutUnit MarginUnderForChild(const LayoutGrid&,
                                        const LayoutBox&,
                                        GridAxis);
  static bool IsOrthogonalChild(const LayoutGrid&, const LayoutBox&);
  static GridTrackSizingDirection FlowAwareDirectionForChild(
      const LayoutGrid&,
      const LayoutBox&,
      GridTrackSizingDirection);
  static bool IsHorizontalGridAxis(const LayoutGrid&, GridAxis);
  static bool IsParallelToBlockAxisForChild(const LayoutGrid&,
                                            const LayoutBox&,
                                            GridAxis);
};

}  // namespace blink

#endif  // GridLayoutUtils_h
