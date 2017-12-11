// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_physical_text_fragment.h"

#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_line_height_metrics.h"
#include "core/style/ComputedStyle.h"

namespace blink {

NGPhysicalOffsetRect NGPhysicalTextFragment::SelfVisualRect() const {
  if (!shape_result_)
    return {};

  // Glyph bounds is in logical coordinate, origin at the alphabetic baseline.
  LayoutRect visual_rect = EnclosingLayoutRect(shape_result_->Bounds());

  // Make the origin at the logical top of this fragment.
  const ComputedStyle& style = Style();
  const SimpleFontData* font_data = style.GetFont().PrimaryFont();
  visual_rect.SetY(visual_rect.Y() + font_data->GetFontMetrics().FixedAscent(
                                         kAlphabeticBaseline));

  if (float stroke_width = style.TextStrokeWidth()) {
    visual_rect.Inflate(LayoutUnit::FromFloatCeil(stroke_width / 2.0f));
  }

  // TODO(kojii): Implement emphasis marks. This needs larger refactoring to
  // share the logic with InlineFlowBox::AddTextBoxVisualOverflow().

  if (ShadowList* text_shadow = style.TextShadow()) {
    LayoutRectOutsets text_shadow_logical_outsets =
        LayoutRectOutsets(text_shadow->RectOutsetsIncludingOriginal())
            .LineOrientationOutsets(style.GetWritingMode());
    text_shadow_logical_outsets.ClampNegativeToZero();
    visual_rect.Expand(text_shadow_logical_outsets);
  }

#if 1
  LayoutRect frame_rect = {{}, Size().ToLayoutSize()};
  visual_rect.Unite(frame_rect);
#endif
  visual_rect = LayoutRect(EnclosingIntRect(visual_rect));

  switch (LineOrientation()) {
    case NGLineOrientation::kHorizontal:
      return NGPhysicalOffsetRect(visual_rect);
    case NGLineOrientation::kClockWiseVertical:
      return {{size_.width - visual_rect.MaxY(), visual_rect.X()},
              {visual_rect.Height(), visual_rect.Width()}};
    case NGLineOrientation::kCounterClockWiseVertical:
      return {{visual_rect.Y(), size_.height - visual_rect.MaxX()},
              {visual_rect.Height(), visual_rect.Width()}};
  }
  NOTREACHED();
  return {};
}

}  // namespace blink
