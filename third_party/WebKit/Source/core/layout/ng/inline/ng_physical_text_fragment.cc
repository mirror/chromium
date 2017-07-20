// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_physical_text_fragment.h"

#include "core/layout/ng/inline/ng_line_height_metrics.h"

namespace blink {

void NGPhysicalTextFragment::ComputeVisualOverflow() const {
  DCHECK(shape_result_);

  // TODO(kojii): Copying InlineTextBox logic from
  // InlineFlowBox::ComputeOverflow().
  //
  // InlineFlowBox::ComputeOverflow() computes GlpyhOverflow first
  // (ComputeGlyphOverflow) then AddTextBoxVisualOverflow(). We probably don't
  // have to keep these two steps separated.

  // Glyph bounds is in logical coordinate, origin at the baseline.
  FloatRect visual_rect = shape_result_->Bounds();

  // Make the origin at the logical top of this fragment.
  const SimpleFontData* font_data = shape_result_->PrimaryFont();
  NGLineHeightMetrics metrics(font_data->GetFontMetrics(), BaselineType());
  visual_rect.SetY(visual_rect.Y() + metrics.ascent);
#if 0
  glyph_overflow_out->SetFromBounds(shape_result_->Bounds(), metrics.ascent,
                                    metrics.descent,
                                    shape_result_->SnappedWidth());
#endif

  // TODO(kojii): Copying from AddTextBoxVisualOverflow()
  const ComputedStyle& style = Style();
  if (float stroke_width = style.TextStrokeWidth()) {
    visual_rect.Inflate(stroke_width / 2.0f);
  }

  // TODO(kojii): Implement emphasis marks.

  // letter-spacing, including negative, is built into ShapeResult.
  // No need to take it into account here.

  if (ShadowList* text_shadow = style.TextShadow()) {
    // TODO(kojii): Implement text shadow.
  }

  // TODO(kojii): Convert to LayoutUnit

  // TODO(kojii): Check if the visual_rect is not contained in this fragment.

  // TODO(kojii): Return...which type? NGLogicalRect? Or set to this
  // NGPhysicalFragment to implement VisualRect()?

  // DisplayItemClient says VisualRect() is "in the space of the parent
  // transform node" for SPv2. Does this mean parent fragment?
}

}  // namespace blink
