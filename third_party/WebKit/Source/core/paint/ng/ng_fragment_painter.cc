// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_fragment_painter.h"

#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutObject.h"
#include "core/layout/LayoutTheme.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/ng/ng_box_fragment_painter.h"
#include "core/paint/ng/ng_paint_fragment.h"
#include "core/paint/ng/ng_text_fragment_painter.h"
#include "core/style/BorderEdge.h"
#include "core/style/ComputedStyle.h"
#include "platform/geometry/LayoutPoint.h"
#include "platform/graphics/GraphicsContextStateSaver.h"
#include "platform/graphics/paint/DrawingRecorder.h"
#include "platform/runtime_enabled_features.h"

namespace blink {

void NGFragmentPainter::PaintAllPhasesAtomically(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  // Create painter based on inline or block?

  // Pass PaintPhaseSelection and PaintPhaseTextClip to the descendants so that
  // they will paint for selection and text clip respectively. We don't need
  // complete painting for these phases.
  PaintPhase phase = paint_info.phase;
  if (phase == PaintPhase::kSelection || phase == PaintPhase::kTextClip)
    return Paint(paint_info, paint_offset);

  if (phase != PaintPhase::kForeground)
    return;

  PaintInfo info(paint_info);
  info.phase = PaintPhase::kBlockBackground;
  Paint(info, paint_offset);
  info.phase = PaintPhase::kFloat;
  Paint(info, paint_offset);
  info.phase = PaintPhase::kForeground;
  Paint(info, paint_offset);
  info.phase = PaintPhase::kOutline;
  Paint(info, paint_offset);
}

void NGFragmentPainter::Paint(const PaintInfo& paint_info,
                              const LayoutPoint& paint_offset) {
  if (fragment_.PhysicalFragment().IsBox())
    NGBoxFragmentPainter(fragment_).Paint(paint_info, paint_offset);
  else if (fragment_.PhysicalFragment().IsText())
    NGTextFragmentPainter(fragment_).Paint(paint_info, paint_offset);
  else
    NOTREACHED();
}

void NGFragmentPainter::PaintOutline(const PaintInfo& paint_info,
                                     const LayoutPoint& paint_offset) {
  DCHECK(ShouldPaintSelfOutline(paint_info.phase));

  const ComputedStyle& style = fragment_.Style();
  if (!style.HasOutline() || style.Visibility() != EVisibility::kVisible)
    return;

  // Only paint the focus ring by hand if the theme isn't able to draw the focus
  // ring.
  if (style.OutlineStyleIsAuto() &&
      !LayoutTheme::GetTheme().ShouldDrawDefaultFocusRing(fragment_.GetNode(),
                                                          style)) {
    return;
  }

  Vector<LayoutRect> outline_rects;
  fragment_.AddOutlineRects(&outline_rects, paint_offset);
  if (outline_rects.IsEmpty())
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(paint_info.context, fragment_,
                                                  paint_info.phase))
    return;

  PaintOutlineRects(paint_info, outline_rects, style, fragment_);
}

void NGFragmentPainter::PaintInlineChildren(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) const {
  DCHECK(!ShouldPaintSelfOutline(paint_info.phase) &&
         !ShouldPaintDescendantOutlines(paint_info.phase));

  // Only paint during the foreground/selection phases.
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection &&
      paint_info.phase != PaintPhase::kTextClip &&
      paint_info.phase != PaintPhase::kMask) {
    return;
  }

  // The only way an inline could paint like this is if it has a layer.
  const NGPhysicalFragment& physical = fragment_.PhysicalFragment();
  DCHECK(physical.IsBox() || (physical.IsLineBox() && physical.HasLayer()));

  if (paint_info.phase == PaintPhase::kForeground && paint_info.IsPrinting()) {
    // AddPDFURLRectsForInlineChildrenRecursively(layout_object, paint_info,
    //                                           paint_offset);
  }

  // NGPhysicalContainerFragment is the base class for both box and line box
  // fragments.
  const NGPhysicalContainerFragment& container_fragment =
      static_cast<const NGPhysicalContainerFragment&>(physical);

  // If we have no lines then we have no work to do.
  if (!container_fragment.Children().size())
    return;

  // TODO(layout-dev): Implement dirty rect comparsion.
  // if (!AnyLineIntersectsRect(...)
  //   return;

  // See if our root lines intersect with the dirty rect. If so, then we paint
  // them. Note that boxes can easily overlap, so we can't make any assumptions
  // based off positions of our first line box or our last line box.
  PaintInfo descendants_info = paint_info.ForDescendants();
  for (const auto& child : fragment_.Children()) {
    // TODO(layout-dev): Implement dirty rect comparsion.
    // if (LineIntersectsDirtyRect(...))

    // PaintInlineChild(*child, paint_info, paint_offset);
    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    if (fragment.Type() == NGPhysicalFragment::kFragmentLineBox) {
      const NGPaintFragment& line_box_fragment = *child;
      for (const auto& line_box_child : line_box_fragment.Children()) {
        PaintInlineChild(
            *line_box_child, descendants_info,
            paint_offset + line_box_fragment.Offset().ToLayoutPoint());
      }
    }
  }
}

void NGFragmentPainter::PaintInlineChildrenOutlines(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) const {}

void NGFragmentPainter::PaintInlineChild(
    const NGPaintFragment& child,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) const {
  const NGPhysicalFragment& fragment = child.PhysicalFragment();
  if (fragment.Type() == NGPhysicalFragment::kFragmentText) {
    PaintTextChild(child, paint_info, paint_offset);
  } else if (fragment.Type() == NGPhysicalFragment::kFragmentBox) {
    if (child.HasSelfPaintingLayer())
      return;

    if (RequiresLegacyFallback(fragment)) {
      PaintInlineChildBoxUsingLegacyFallback(fragment, paint_info,
                                             paint_offset);

    } else {
      NGBoxFragmentPainter(child).PaintInlineBox(paint_info, paint_offset,
                                                 paint_offset);
    }
  }
}

void NGFragmentPainter::PaintInlineChildBoxUsingLegacyFallback(
    const NGPhysicalFragment& fragment,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) const {
  LayoutObject* layout_object = fragment.GetLayoutObject();
  DCHECK(layout_object);
  if (layout_object->IsLayoutNGMixin() &&
      ToLayoutBlockFlow(layout_object)->PaintFragment()) {
    // This object will use NGBoxFragmentPainter. NGBoxFragmentPainter expects
    // |paint_offset| relative to the parent, even when in inline context.
    layout_object->Paint(paint_info, paint_offset);
    return;
  }

  // When in inline context, pre-NG painters expect |paint_offset| of their
  // block container.
  if (layout_object->IsAtomicInlineLevel()) {
    // Pre-NG painters also expect callers to use |PaintAllPhasesAtomically()|
    // for atomic inlines.
    ObjectPainter(*layout_object)
        .PaintAllPhasesAtomically(paint_info,
                                  paint_offset);  // block_paint_offset_?
    return;
  }

  layout_object->Paint(paint_info, paint_offset);  // block_paint_offset_?
}

void NGFragmentPainter::PaintTextChild(const NGPaintFragment& text_fragment,
                                       const PaintInfo& paint_info,
                                       const LayoutPoint& paint_offset) const {
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, text_fragment,
          DisplayItem::PaintPhaseToDrawingType(paint_info.phase)))
    return;

  DrawingRecorder recorder(
      paint_info.context, text_fragment,
      DisplayItem::PaintPhaseToDrawingType(paint_info.phase));

  NGTextFragmentPainter text_painter(text_fragment);
  text_painter.Paint(paint_info, paint_offset);
}

bool NGFragmentPainter::RequiresLegacyFallback(
    const NGPhysicalFragment& fragment) {
  // Fallback to LayoutObject if this is a root of NG block layout.
  // If this box is for this painter, LayoutNGBlockFlow will call back.
  if (fragment.IsBlockLayoutRoot())
    return true;

  // TODO(kojii): Review if this is still needed.
  LayoutObject* layout_object = fragment.GetLayoutObject();
  return layout_object->IsLayoutReplaced();
}

}  // namespace blink
