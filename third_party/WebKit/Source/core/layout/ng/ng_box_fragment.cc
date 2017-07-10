// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_box_fragment.h"

#include "core/layout/LayoutBox.h"
#include "core/layout/ng/geometry/ng_logical_size.h"
#include "core/layout/ng/inline/ng_line_height_metrics.h"
#include "core/layout/ng/ng_macros.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

NGLogicalSize NGBoxFragment::OverflowSize() const {
  auto* physical_fragment = ToNGPhysicalBoxFragment(physical_fragment_);
  return physical_fragment->OverflowSize().ConvertToLogical(WritingMode());
}

NGLineHeightMetrics NGBoxFragment::BaselineMetrics(
    NGBaselineAlgorithmType algorithm_type,
    FontBaseline baseline_type) const {
  const NGPhysicalBoxFragment* physical_fragment =
      ToNGPhysicalBoxFragment(physical_fragment_);
  LayoutObject* layout_object = physical_fragment->GetLayoutObject();
#if 1
  for (const auto& baseline : physical_fragment->Baselines()) {
    if (baseline.algorithm_type == algorithm_type &&
        baseline.baseline_type == baseline_type) {
      LayoutUnit descent = BlockSize() - baseline.offset;

      // If atomic inline, use the margin edge.
      // For replaced elements, inline-block elements, and inline-table
      // elements, the height is the height of their margin box.
      // https://drafts.csswg.org/css2/visudet.html#line-height
      if (layout_object->IsAtomicInlineLevel()) {
        LayoutBox* layout_box = ToLayoutBox(layout_object);
        return NGLineHeightMetrics(baseline.offset + layout_box->MarginOver(),
                                   descent + layout_box->MarginUnder());
      }

      return NGLineHeightMetrics(baseline.offset, descent);
    }
  }

  LayoutUnit block_size = BlockSize();

  // If atomic inline, use the margin edge. See above.
  if (layout_object->IsAtomicInlineLevel())
    block_size += ToLayoutBox(layout_object)->MarginLogicalHeight();

  if (baseline_type == kAlphabeticBaseline)
    return NGLineHeightMetrics(block_size, LayoutUnit());
  return NGLineHeightMetrics(block_size - block_size / 2, block_size / 2);
#else
  NGBaselineSource baseline_source(physical_fragment,
                                   ConstraintSpace().WritingMode(),
                                   line_info.UseFirstLineStyle());
  LayoutUnit baseline_offset = baseline_source.BaselinePosition(baseline_type_);
#endif
}

}  // namespace blink
