// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/geometry/ng_replaced_size.h"

#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_length_utils.h"
#include "core/style/ComputedStyle.h"

namespace blink {

NGPhysicalBoxStrut ComputePhysicalBorderPadding(const NGConstraintSpace& space,
                                                const ComputedStyle& style) {
  return (ComputeBorders(space, style) + ComputePadding(space, style))
      .ConvertToPhysical(space.WritingMode(), space.Direction());
}

LayoutUnit NGReplacedSize::ComputeReplacedWidth(
    const NGConstraintSpace& space,
    const ComputedStyle& style,
    const Optional<MinMaxContentSize>& child_minmax) const {
  LayoutUnit computed_width;
  Length width = style.Width();
  Length height = style.Height();
  if (width.IsAuto()) {
    if (height.IsAuto() || !aspect_ratio.has_value()) {
      // Use intrinsic values if width cannot be computed from height and
      // aspect ratio.
      if (content_width.has_value()) {
        // In some cases (svg?) width has been precomputed.
        computed_width = content_width.value();
      } else {
        computed_width = intrinsic_content_size.width;
      }
      computed_width +=
          ComputePhysicalBorderPadding(space, style).HorizontalSum();
    } else {
      // Width can be computed from height and aspect ratio.
      computed_width = ResolveHeight(height, space, style, child_minmax,
                                     LengthResolveType::kContentSize) *
                       aspect_ratio.value().width / aspect_ratio.value().height;
    }
  } else {
    computed_width = ResolveWidth(width, space, style, child_minmax,
                                  LengthResolveType::kContentSize);
  }
  return computed_width;
}

// This function is just like ComputeReplacedWidth, but for Height.
LayoutUnit NGReplacedSize::ComputeReplacedHeight(
    const NGConstraintSpace& space,
    const ComputedStyle& style,
    const Optional<MinMaxContentSize>& child_minmax) const {
  LayoutUnit computed_height;
  Length width = style.Width();
  Length height = style.Height();
  if (height.IsAuto()) {
    if (width.IsAuto() || !aspect_ratio.has_value()) {
      // Use intrinsic values if height cannot be computed from width and
      // aspect ratio.
      if (content_height.has_value()) {
        computed_height = content_height.value();
      } else {
        computed_height = intrinsic_content_size.height;
      }
      computed_height +=
          ComputePhysicalBorderPadding(space, style).VerticalSum();
    } else {
      // Height can be computed from width and aspect ratio.
      computed_height = ResolveWidth(width, space, style, child_minmax,
                                     LengthResolveType::kContentSize) *
                        aspect_ratio.value().height /
                        aspect_ratio.value().width;
    }
  } else {
    computed_height = ResolveHeight(height, space, style, child_minmax,
                                    LengthResolveType::kContentSize);
  }
  return computed_height;
}

}  // namespace blink
