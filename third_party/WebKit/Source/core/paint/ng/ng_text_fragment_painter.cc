// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_text_fragment_painter.h"

#include "core/editing/FrameSelection.h"
#include "core/frame/LocalFrame.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/layout/ng/ng_text_decoration_offset.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/SelectionPaintingUtils.h"
#include "core/paint/TextPainterBase.h"
#include "core/paint/ng/ng_paint_fragment.h"
#include "core/paint/ng/ng_text_painter.h"
#include "core/style/AppliedTextDecoration.h"
#include "core/style/ComputedStyle.h"
#include "platform/graphics/GraphicsContextStateSaver.h"

namespace blink {

NGTextFragmentPainter::NGTextFragmentPainter(
    const NGPaintFragment& text_fragment)
    : fragment_(text_fragment) {
  DCHECK(text_fragment.PhysicalFragment().IsText());
}

static unsigned clampOffset(unsigned offset,
                            const NGPhysicalTextFragment& text_fragment) {
  return std::min(std::max(offset, text_fragment.StartOffset()),
                  text_fragment.EndOffset());
}

static std::pair<unsigned, unsigned> GetSelectionStartEndInFragment(
    const Document& document,
    const NGPhysicalTextFragment& text_fragment) {
  // FrameSelection holds selection offsets in layout block flow at
  // LayoutSelection::Commit().
  // Unlike legacy layout tree, LayoutNGBlockFlow has only one atomic
  // SelectionState.
  const FrameSelection& selection = document.GetFrame()->Selection();
  switch (text_fragment.GetLayoutObject()
              ->EnclosingNGBlockFlow()
              ->GetSelectionState()) {
    case SelectionState::kStart: {
      unsigned start_in_block =
          (unsigned)selection.LayoutSelectionStart().value_or(0);
      return {clampOffset(start_in_block, text_fragment),
              text_fragment.EndOffset()};
    }
    case SelectionState::kEnd: {
      unsigned end_in_block =
          (unsigned)selection.LayoutSelectionEnd().value_or(0);
      return {text_fragment.StartOffset(),
              clampOffset(end_in_block, text_fragment)};
    }
    case SelectionState::kStartAndEnd: {
      unsigned start_in_block =
          (unsigned)selection.LayoutSelectionStart().value_or(0);
      unsigned end_in_block =
          (unsigned)selection.LayoutSelectionEnd().value_or(0);
      return {clampOffset(start_in_block, text_fragment),
              clampOffset(end_in_block, text_fragment)};
    }
    case SelectionState::kInside:
      return {text_fragment.StartOffset(), text_fragment.EndOffset()};
    default:
      // This block is not included in selection.
      return {0, 0};
  }
}

static void PaintSelection(const Document& document,
                           const NGPhysicalTextFragment& text_fragment,
                           const ComputedStyle& style,
                           GraphicsContext& context,
                           const LayoutRect& box_rect,
                           const Font& font,
                           Color text_color,
                           unsigned int selection_start,
                           unsigned int selection_end) {
  Color c = SelectionPaintingUtils::SelectionBackgroundColor(
      document, style, text_fragment.GetNode());
  if (!c.Alpha())
    return;

  // If the text color ends up being the same as the selection background,
  // invert the selection background.
  if (text_color == c)
    c = Color(0xff - c.Red(), 0xff - c.Green(), 0xff - c.Blue());

  TextRun text_run = TextRun(text_fragment.Text(), box_rect.X().ToFloat());
  GraphicsContextStateSaver state_saver(context);
  DCHECK_NE(text_fragment.StartOffset(), selection_start);
  DCHECK_NE(text_fragment.StartOffset(), selection_end);
  LayoutRect selection_rect = LayoutRect(font.SelectionRectForText(
      text_run, FloatPoint(box_rect.Location()), box_rect.Height().ToFloat(),
      selection_start - text_fragment.StartOffset(),
      selection_end - text_fragment.StartOffset()));

  context.FillRect(FloatRect(selection_rect), c);
}

void NGTextFragmentPainter::Paint(const Document& document,
                                  const PaintInfo& paint_info,
                                  const LayoutPoint& paint_offset) {
  const ComputedStyle& style = fragment_.Style();

  NGPhysicalSize size_;
  NGPhysicalOffset offset_;

  // We round the y-axis to ensure consistent line heights.
  LayoutPoint adjusted_paint_offset =
      LayoutPoint(paint_offset.X(), LayoutUnit(paint_offset.Y().Round()));

  NGPhysicalOffset offset = fragment_.Offset();
  LayoutPoint box_origin(offset.left, offset.top);
  box_origin.Move(adjusted_paint_offset.X(), adjusted_paint_offset.Y());

  LayoutRect box_rect(
      box_origin, LayoutSize(fragment_.Size().width, fragment_.Size().height));

  GraphicsContext& context = paint_info.context;

  bool is_printing = paint_info.IsPrinting();

  // Determine whether or not we're selected.
  const NGPhysicalTextFragment& text_fragment =
      ToNGPhysicalTextFragment(fragment_.PhysicalFragment());
  int selection_start;
  int selection_end;
  std::tie(selection_start, selection_end) =
      GetSelectionStartEndInFragment(document, text_fragment);
  const bool have_selection = selection_start < selection_end;
  if (!have_selection && paint_info.phase == PaintPhase::kSelection) {
    // When only painting the selection, don't bother to paint if there is none.
    return;
  }

  // Determine text colors.
  TextPaintStyle text_style =
      TextPainterBase::TextPaintingStyle(document, style, paint_info);
  TextPaintStyle selection_style = TextPainterBase::SelectionPaintingStyle(
      document, style, fragment_.GetNode(), have_selection, paint_info,
      text_style);
  bool paint_selected_text_only = (paint_info.phase == PaintPhase::kSelection);
  bool paint_selected_text_separately =
      !paint_selected_text_only && text_style != selection_style;

  // Set our font.
  const Font& font = style.GetFont();
  const SimpleFontData* font_data = font.PrimaryFont();
  DCHECK(font_data);

  int ascent = font_data ? font_data->GetFontMetrics().Ascent() : 0;
  LayoutPoint text_origin(box_origin.X(), box_origin.Y() + ascent);

  // 1. Paint backgrounds behind text if needed. Examples of such backgrounds
  // include selection and composition highlights.

  if (have_selection && paint_info.phase != PaintPhase::kSelection &&
      paint_info.phase != PaintPhase::kTextClip && !is_printing) {
    PaintSelection(document, text_fragment, style, context, box_rect, font,
                   selection_style.fill_color, selection_start, selection_end);
  }

  // 2. Now paint the foreground, including text and decorations.
  NGTextPainter text_painter(context, font, text_fragment, text_origin,
                             box_rect, text_fragment.IsHorizontal());

  if (style.GetTextEmphasisMark() != TextEmphasisMark::kNone) {
    text_painter.SetEmphasisMark(style.TextEmphasisMarkString(),
                                 style.GetTextEmphasisPosition());
  }

  unsigned length = text_fragment.Text().length();
  if (!paint_selected_text_only) {
    // Paint text decorations except line-through.
    DecorationInfo decoration_info;
    bool has_line_through_decoration = false;
    if (style.TextDecorationsInEffect() != TextDecoration::kNone) {
      LayoutPoint local_origin = LayoutPoint(box_origin);
      LayoutUnit width = fragment_.Size().width;
      const NGPhysicalBoxFragment* decorating_box = nullptr;
      const ComputedStyle* decorating_box_style =
          decorating_box ? &decorating_box->Style() : nullptr;

      // TODO(eae): Use correct baseline when available.
      FontBaseline baseline_type = kAlphabeticBaseline;

      text_painter.ComputeDecorationInfo(decoration_info, box_origin,
                                         local_origin, width, baseline_type,
                                         style, decorating_box_style);

      NGTextDecorationOffset decoration_offset(*decoration_info.style,
                                               text_fragment, decorating_box);
      text_painter.PaintDecorationsExceptLineThrough(
          decoration_offset, decoration_info, paint_info,
          style.AppliedTextDecorations(), text_style,
          &has_line_through_decoration);
    }

    int start_offset = text_fragment.StartOffset();
    int end_offset = start_offset + length;

    if (have_selection && paint_selected_text_separately) {
      // Paint only the text that is not selected.
      if (start_offset < selection_start)
        text_painter.Paint(start_offset, selection_start, length, text_style);
      if (selection_end < end_offset)
        text_painter.Paint(selection_end, end_offset, length, text_style);
    } else {
      text_painter.Paint(start_offset, end_offset, length, text_style);
    }

    // Paint line-through decoration if needed.
    if (has_line_through_decoration) {
      text_painter.PaintDecorationsOnlyLineThrough(
          decoration_info, paint_info, style.AppliedTextDecorations(),
          text_style);
    }
  }

  if (have_selection &&
      (paint_selected_text_only || paint_selected_text_separately)) {
    // Paint only the text that is selected.
    text_painter.Paint(selection_start, selection_end, length, selection_style);
  }
}

}  // namespace blink
