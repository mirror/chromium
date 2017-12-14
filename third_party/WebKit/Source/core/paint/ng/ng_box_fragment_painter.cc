// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_box_fragment_painter.h"

#include "core/layout/BackgroundBleedAvoidance.h"
#include "core/layout/HitTestLocation.h"
#include "core/layout/HitTestResult.h"
#include "core/layout/ng/geometry/ng_border_edges.h"
#include "core/layout/ng/geometry/ng_box_strut.h"
#include "core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/layout_ng_mixin.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/paint/AdjustPaintOffsetScope.h"
#include "core/paint/BackgroundImageGeometry.h"
#include "core/paint/BoxDecorationData.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/PaintLayer.h"
#include "core/paint/PaintPhase.h"
#include "core/paint/ScrollRecorder.h"
#include "core/paint/ng/ng_fragment_painter.h"
#include "core/paint/ng/ng_paint_fragment.h"
#include "core/paint/ng/ng_text_fragment_painter.h"
#include "platform/geometry/LayoutRectOutsets.h"
#include "platform/graphics/GraphicsContextStateSaver.h"
#include "platform/graphics/paint/DrawingRecorder.h"
#include "platform/scroll/ScrollTypes.h"

namespace blink {

namespace {
LayoutRectOutsets BoxStrutToLayoutRectOutsets(
    const NGPixelSnappedPhysicalBoxStrut& box_strut) {
  return LayoutRectOutsets(
      LayoutUnit(box_strut.top), LayoutUnit(box_strut.right),
      LayoutUnit(box_strut.bottom), LayoutUnit(box_strut.left));
}
}  // anonymous namespace

NGBoxFragmentPainter::NGBoxFragmentPainter(const NGPaintFragment& box)
    : BoxPainterBase(
          box,
          &box.GetLayoutObject()->GetDocument(),
          box.Style(),
          box.GetLayoutObject()->GeneratingNode(),
          BoxStrutToLayoutRectOutsets(box.PhysicalFragment().BorderWidths()),
          LayoutRectOutsets()),
      box_fragment_(box),
      border_edges_(
          NGBorderEdges::FromPhysical(box.PhysicalFragment().BorderEdges(),
                                      box.Style().GetWritingMode())),
      is_inline_(false) {
  DCHECK(box.PhysicalFragment().IsBox());
}

void NGBoxFragmentPainter::Paint(const PaintInfo& paint_info,
                                 const LayoutPoint& paint_offset) {
  const NGPhysicalFragment& fragment = box_fragment_.PhysicalFragment();
  const LayoutObject* layout_object = fragment.GetLayoutObject();
  DCHECK(layout_object && layout_object->IsBox());
  if (!fragment.IsPlacedByLayoutNG()) {
    // |fragment.Offset()| is valid only when it is placed by LayoutNG parent.
    // Use LayoutBox::Location() if not. crbug.com/788590
    AdjustPaintOffsetScope adjustment(ToLayoutBox(*layout_object), paint_info,
                                      paint_offset);
    PaintWithAdjustedOffset(adjustment.MutablePaintInfo(),
                            adjustment.AdjustedPaintOffset());
    return;
  }

  AdjustPaintOffsetScope adjustment(box_fragment_, paint_info, paint_offset);
  PaintWithAdjustedOffset(adjustment.MutablePaintInfo(),
                          adjustment.AdjustedPaintOffset());
}

void NGBoxFragmentPainter::PaintInlineBox(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutPoint& block_paint_offset) {
  CHECK(!box_fragment_.PhysicalFragment().IsFloating());
  DCHECK(box_fragment_.GetLayoutObject() &&
         box_fragment_.GetLayoutObject()->IsLayoutInline());
  is_inline_ = true;
  block_paint_offset_ = block_paint_offset;
  PaintInfo info(paint_info);
  PaintWithAdjustedOffset(
      info, paint_offset + box_fragment_.Offset().ToLayoutPoint());
}

void NGBoxFragmentPainter::PaintWithAdjustedOffset(
    PaintInfo& info,
    const LayoutPoint& paint_offset) {
  if (!IntersectsPaintRect(info, paint_offset))
    return;

  if (box_fragment_.PhysicalFragment().IsInlineBlock())
    return PaintInlineBlock(info, paint_offset);

  if (box_fragment_.PhysicalFragment().IsFloating())
    return;

  PaintPhase original_phase = info.phase;

  if (original_phase == PaintPhase::kOutline) {
    info.phase = PaintPhase::kDescendantOutlinesOnly;
  } else if (ShouldPaintSelfBlockBackground(original_phase)) {
    info.phase = PaintPhase::kSelfBlockBackgroundOnly;
    PaintBlock(info, paint_offset);
    if (ShouldPaintDescendantBlockBackgrounds(original_phase))
      info.phase = PaintPhase::kDescendantBlockBackgroundsOnly;
  }

  if (original_phase != PaintPhase::kSelfBlockBackgroundOnly &&
      original_phase != PaintPhase::kSelfOutlineOnly) {
    // TODO(layout-dev): Clip using BoxClipper.
    // BoxClipper box_clipper(layout_block_, info, paint_offset,
    //                       contents_clip_behavior);
    PaintBlock(info, paint_offset);
  }

  if (ShouldPaintSelfOutline(original_phase)) {
    info.phase = PaintPhase::kSelfOutlineOnly;
    PaintBlock(info, paint_offset);
  }

  // Our scrollbar widgets paint exactly when we tell them to, so that they work
  // properly with z-index. We paint after we painted the background/border, so
  // that the scrollbars will sit above the background/border.
  info.phase = original_phase;
  PaintOverflowControlsIfNeeded(info, paint_offset);
}

void NGBoxFragmentPainter::PaintBlock(const PaintInfo& paint_info,
                                      const LayoutPoint& paint_offset) {
  const PaintPhase paint_phase = paint_info.phase;
  const ComputedStyle& style = box_fragment_.Style();
  bool is_visible = style.Visibility() == EVisibility::kVisible;

  if (ShouldPaintSelfBlockBackground(paint_phase)) {
    // TODO(layout-dev): style.HasBoxDecorationBackground isn't good enough, it
    // needs to check the object as some objects may have box decoration
    // background other than from their own style.
    if (is_visible && style.HasBoxDecorationBackground())
      PaintBoxDecorationBackground(paint_info, paint_offset);
    // Record the scroll hit test after the background so background squashing
    // is not affected. Hit test order would be equivalent if this were
    // immediately before the background.
    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
      PaintScrollHitTestDisplayItem(paint_info);
    // We're done. We don't bother painting any children.
    if (paint_phase == PaintPhase::kSelfBlockBackgroundOnly)
      return;
  }

  if (paint_info.PaintRootBackgroundOnly())
    return;

  if (paint_phase == PaintPhase::kMask && is_visible) {
    PaintMask(paint_info, paint_offset);
    return;
  }

  if (paint_phase == PaintPhase::kClippingMask && is_visible) {
    // SPv175 always paints clipping mask in PaintLayerPainter.
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
    PaintClippingMask(paint_info, paint_offset);
    return;
  }

  if (paint_phase == PaintPhase::kForeground && paint_info.IsPrinting()) {
    // ObjectPainter(layout_block_)
    //    .AddPDFURLRectIfNeeded(paint_info, paint_offset);
  }

  if (paint_phase != PaintPhase::kSelfOutlineOnly) {
    Optional<ScopedPaintChunkProperties> scoped_scroll_property;
    Optional<ScrollRecorder> scroll_recorder;
    Optional<PaintInfo> scrolled_paint_info;
    DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
    // TODO(layout-dev): Figure out where paint properties should live.
    const auto* object_properties =
        box_fragment_.GetLayoutObject()->FirstFragment().PaintProperties();
    auto* scroll_translation =
        object_properties ? object_properties->ScrollTranslation() : nullptr;
    if (scroll_translation) {
      scoped_scroll_property.emplace(
          paint_info.context.GetPaintController(), scroll_translation,
          box_fragment_, DisplayItem::PaintPhaseToDrawingType(paint_phase));
      scrolled_paint_info.emplace(paint_info);
      scrolled_paint_info->UpdateCullRectForScrollingContents(
          EnclosingIntRect(box_fragment_.OverflowClipRect(paint_offset)),
          scroll_translation->Matrix().ToAffineTransform());
    }

    const PaintInfo& contents_paint_info =
        scrolled_paint_info ? *scrolled_paint_info : paint_info;

    if (box_fragment_.PhysicalFragment().IsBlockFlow()) {
      PaintBlockFlowContents(contents_paint_info, paint_offset);
      if (paint_phase == PaintPhase::kFloat ||
          paint_phase == PaintPhase::kSelection ||
          paint_phase == PaintPhase::kTextClip)
        PaintFloats(contents_paint_info, paint_offset);
    } else {
      PaintContents(contents_paint_info, paint_offset);
    }
  }

  if (ShouldPaintSelfOutline(paint_phase))
    NGFragmentPainter(box_fragment_).PaintOutline(paint_info, paint_offset);

  // If the caret's node's layout object's containing block is this block, and
  // the paint action is PaintPhaseForeground, then paint the caret.
  // if (paint_phase == PaintPhase::kForeground &&
  //    layout_block_.ShouldPaintCarets())
  // PaintCarets(paint_info, paint_offset);
}

void NGBoxFragmentPainter::PaintBlockFlowContents(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {

  // Avoid painting descendants of the root element when stylesheets haven't
  // loaded. This eliminates FOUC.  It's ok not to draw, because later on, when
  // all the stylesheets do load, styleResolverMayHaveChanged() on Document will
  // trigger a full paint invalidation.
  // TODO(layout-dev): Handle without delegating to LayoutObject.
  LayoutObject* layout_object = box_fragment_.GetLayoutObject();
  if (layout_object->GetDocument().DidLayoutWithPendingStylesheets() &&
      !layout_object->IsLayoutView()) {
    return;
  }

  const NGPhysicalBoxFragment& physical =
      static_cast<const NGPhysicalBoxFragment&>(
          box_fragment_.PhysicalFragment());
  if (!physical.ChildrenInline())
    return PaintContents(paint_info, paint_offset);

  if (ShouldPaintDescendantOutlines(paint_info.phase)) {
    NGFragmentPainter(box_fragment_)
        .PaintInlineChildrenOutlines(paint_info, paint_offset);
  } else {
    NGFragmentPainter(box_fragment_)
        .PaintInlineChildren(paint_info, paint_offset);
  }
}

void NGBoxFragmentPainter::PaintInlineObject(const PaintInfo& paint_info,
                                             const LayoutPoint& paint_offset) {
  DCHECK(!ShouldPaintSelfOutline(paint_info.phase) &&
         !ShouldPaintDescendantOutlines(paint_info.phase));

  LayoutRect overflow_rect(box_fragment_.VisualOverflowRect());
  overflow_rect.MoveBy(paint_offset);

  if (!paint_info.GetCullRect().IntersectsCullRect(overflow_rect))
    return;

  if (paint_info.phase == PaintPhase::kMask) {
    if (DrawingRecorder::UseCachedDrawingIfPossible(
            paint_info.context, box_fragment_,
            DisplayItem::PaintPhaseToDrawingType(paint_info.phase)))
      return;
    DrawingRecorder recorder(
        paint_info.context, box_fragment_,
        DisplayItem::PaintPhaseToDrawingType(paint_info.phase));
    PaintMask(paint_info, paint_offset);
    return;
  }

  // Paint our background, border and box-shadow.
  if (paint_info.phase == PaintPhase::kForeground)
    PaintBoxDecorationBackground(paint_info, paint_offset);

  // Paint our children.
  PaintContents(paint_info, paint_offset);
}

void NGBoxFragmentPainter::PaintContents(const PaintInfo& paint_info,
                                         const LayoutPoint& paint_offset) {
  PaintInfo descendants_info = paint_info.ForDescendants();
  if (is_inline_) {
    PaintInlineChildren(box_fragment_.Children(), descendants_info,
                        paint_offset);
    return;
  }
  PaintChildren(box_fragment_.Children(), descendants_info, paint_offset);
}

void NGBoxFragmentPainter::PaintFloatingChildren(
    const Vector<std::unique_ptr<NGPaintFragment>>& children,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  for (const auto& child : children) {
    if (child->HasSelfPaintingLayer())
      continue;

    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    if (fragment.IsFloating()) {
      NGFragmentPainter(*child).PaintAllPhasesAtomically(
          paint_info, paint_offset + fragment.Offset().ToLayoutPoint());
    }

    else if (fragment.Type() == NGPhysicalFragment::kFragmentBox ||
             fragment.Type() == NGPhysicalFragment::kFragmentLineBox) {
      PaintFloatingChildren(child->Children(), paint_info, paint_offset);
    }
  }
}

void NGBoxFragmentPainter::PaintFloats(const PaintInfo& paint_info,
                                       const LayoutPoint& paint_offset) {
  DCHECK(paint_info.phase == PaintPhase::kFloat ||
         paint_info.phase == PaintPhase::kSelection ||
         paint_info.phase == PaintPhase::kTextClip);
  PaintInfo float_info(paint_info);
  if (paint_info.phase == PaintPhase::kFloat)
    float_info.phase = PaintPhase::kForeground;
  PaintFloatingChildren(box_fragment_.Children(), float_info, paint_offset);
}

void NGBoxFragmentPainter::PaintMask(const PaintInfo&, const LayoutPoint&) {
  // TODO(eae): Implement.
}

void NGBoxFragmentPainter::PaintClippingMask(const PaintInfo&,
                                             const LayoutPoint&) {
  // TODO(eae): Implement.
}

void NGBoxFragmentPainter::PaintBoxDecorationBackground(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  LayoutRect paint_rect;
  if (!IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          box_fragment_, paint_info)) {
    // TODO(eae): We need better converters for ng geometry types. Long term we
    // probably want to change the paint code to take NGPhysical* but that is a
    // much bigger change.
    NGPhysicalSize size = box_fragment_.Size();
    paint_rect = LayoutRect(LayoutPoint(), LayoutSize(size.width, size.height));
  }

  paint_rect.MoveBy(paint_offset);
  PaintBoxDecorationBackgroundWithRect(paint_info, paint_offset, paint_rect);
}

void NGBoxFragmentPainter::PaintBoxDecorationBackgroundWithRect(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutRect& paint_rect) {
  bool painting_overflow_contents =
      IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          box_fragment_, paint_info);
  const ComputedStyle& style = box_fragment_.Style();

  // TODO(layout-dev): Implement support for painting overflow contents.
  const DisplayItemClient& display_item_client = box_fragment_;
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, display_item_client,
          DisplayItem::kBoxDecorationBackground))
    return;

  DrawingRecorder recorder(paint_info.context, display_item_client,
                           DisplayItem::kBoxDecorationBackground);
  BoxDecorationData box_decoration_data(box_fragment_.PhysicalFragment());
  GraphicsContextStateSaver state_saver(paint_info.context, false);

  if (!painting_overflow_contents) {
    PaintNormalBoxShadow(paint_info, paint_rect, style);

    if (BleedAvoidanceIsClipping(box_decoration_data.bleed_avoidance)) {
      state_saver.Save();
      FloatRoundedRect border = style.GetRoundedBorderFor(
          paint_rect, border_edges_.line_left, border_edges_.line_right);
      paint_info.context.ClipRoundedRect(border);

      if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer)
        paint_info.context.BeginLayer();
    }
  }

  // TODO(layout-dev): Support theme painting.

  // TODO(eae): Support SkipRootBackground painting.
  bool should_paint_background = true;
  if (should_paint_background) {
    PaintBackground(paint_info, paint_rect,
                    box_decoration_data.background_color,
                    box_decoration_data.bleed_avoidance);
  }

  if (!painting_overflow_contents) {
    PaintInsetBoxShadowWithBorderRect(paint_info, paint_rect, style);

    if (box_decoration_data.has_border_decoration) {
      Node* generating_node = box_fragment_.GetLayoutObject()->GeneratingNode();
      const Document& document = box_fragment_.GetLayoutObject()->GetDocument();
      PaintBorder(box_fragment_, document, generating_node, paint_info,
                  paint_rect, style, box_decoration_data.bleed_avoidance,
                  border_edges_.line_left, border_edges_.line_right);
    }
  }

  if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer)
    paint_info.context.EndLayer();
}

void NGBoxFragmentPainter::PaintChildren(
    const Vector<std::unique_ptr<NGPaintFragment>>& children,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  for (const auto& child : children) {
    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    if (fragment.Type() == NGPhysicalFragment::kFragmentBox) {
      if (child->HasSelfPaintingLayer())
        continue;

      if (NGFragmentPainter::RequiresLegacyFallback(fragment))
        fragment.GetLayoutObject()->Paint(paint_info, paint_offset);
      else
        NGBoxFragmentPainter(*child).Paint(paint_info, paint_offset);
    } else if (fragment.Type() == NGPhysicalFragment::kFragmentLineBox) {
      PaintLineBox(*child, paint_info, paint_offset);
    }
  }
}

void NGBoxFragmentPainter::PaintInlineChildren(
    const Vector<std::unique_ptr<NGPaintFragment>>& children,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  NGFragmentPainter fragment_painter(box_fragment_);
  for (const auto& child : children)
    fragment_painter.PaintInlineChild(*child, paint_info, paint_offset);
}

void NGBoxFragmentPainter::PaintLineBox(
    const NGPaintFragment& line_box_fragment,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  // Only paint during the foreground/selection phases.
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection &&
      paint_info.phase != PaintPhase::kTextClip &&
      paint_info.phase != PaintPhase::kMask)
    return;

  // Line box fragments don't have LayoutObject and nothing to paint. Accumulate
  // its offset and paint children.
  block_paint_offset_ = paint_offset;
  PaintInlineChildren(
      line_box_fragment.Children(), paint_info,
      paint_offset + line_box_fragment.Offset().ToLayoutPoint());
}

void NGBoxFragmentPainter::PaintInlineBlock(const PaintInfo& paint_info,
                                            const LayoutPoint& paint_offset) {
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection)
    return;

  // Text clips are painted only for the direct inline children of the object
  // that has a text clip style on it, not block children.
  DCHECK(paint_info.phase != PaintPhase::kTextClip);

  NGFragmentPainter(box_fragment_)
      .PaintAllPhasesAtomically(paint_info, paint_offset);
}

bool NGBoxFragmentPainter::
    IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
        const NGPaintFragment& fragment,
        const PaintInfo& paint_info) {
  // TODO(layout-dev): Implement once we have support for scrolling.
  return false;
}

void NGBoxFragmentPainter::PaintOverflowControlsIfNeeded(const PaintInfo&,
                                                         const LayoutPoint&) {
  // TODO(layout-dev): Implement once we have support for scrolling.
}

bool NGBoxFragmentPainter::IntersectsPaintRect(
    const PaintInfo& paint_info,
    const LayoutPoint& adjusted_paint_offset) const {
  // TODO(layout-dev): Add support for scrolling, see
  // BlockPainter::IntersectsPaintRect.
  LayoutRect overflow_rect(box_fragment_.VisualOverflowRect());
  overflow_rect.MoveBy(adjusted_paint_offset);
  return paint_info.GetCullRect().IntersectsCullRect(overflow_rect);
}

void NGBoxFragmentPainter::PaintFillLayerTextFillBox(
    GraphicsContext& context,
    const BoxPainterBase::FillLayerInfo& info,
    Image* image,
    SkBlendMode composite_op,
    const BackgroundImageGeometry& geometry,
    const LayoutRect& rect,
    LayoutRect scrolled_paint_rect) {
  // First figure out how big the mask has to be. It should be no bigger
  // than what we need to actually render, so we should intersect the dirty
  // rect with the border box of the background.
  IntRect mask_rect = PixelSnappedIntRect(rect);

  // We draw the background into a separate layer, to be later masked with
  // yet another layer holding the text content.
  GraphicsContextStateSaver background_clip_state_saver(context, false);
  background_clip_state_saver.Save();
  context.Clip(mask_rect);
  context.BeginLayer();

  PaintFillLayerBackground(context, info, image, composite_op, geometry,
                           scrolled_paint_rect);

  // Create the text mask layer and draw the text into the mask. We do this by
  // painting using a special paint phase that signals to InlineTextBoxes that
  // they should just add their contents to the clip.
  context.BeginLayer(1, SkBlendMode::kDstIn);
  PaintInfo paint_info(context, mask_rect, PaintPhase::kTextClip,
                       kGlobalPaintNormalPhase, 0);

  // TODO(eae): Paint text child fragments.

  context.EndLayer();  // Text mask layer.
  context.EndLayer();  // Background layer.
}

LayoutRect NGBoxFragmentPainter::AdjustForScrolledContent(
    const PaintInfo&,
    const BoxPainterBase::FillLayerInfo&,
    const LayoutRect& rect) {
  return rect;
}

BoxPainterBase::FillLayerInfo NGBoxFragmentPainter::GetFillLayerInfo(
    const Color& color,
    const FillLayer& bg_layer,
    BackgroundBleedAvoidance bleed_avoidance) const {
  return BoxPainterBase::FillLayerInfo(
      box_fragment_.GetLayoutObject()->GetDocument(), box_fragment_.Style(),
      box_fragment_.HasOverflowClip(), color, bg_layer, bleed_avoidance,
      border_edges_.line_left, border_edges_.line_right);
}

void NGBoxFragmentPainter::PaintBackground(
    const PaintInfo& paint_info,
    const LayoutRect& paint_rect,
    const Color& background_color,
    BackgroundBleedAvoidance bleed_avoidance) {
  // TODO(eae): Switch to LayoutNG version of BackgroundImageGeometry.
  BackgroundImageGeometry geometry(*static_cast<const LayoutBoxModelObject*>(
      box_fragment_.GetLayoutObject()));
  PaintFillLayers(paint_info, background_color,
                  box_fragment_.Style().BackgroundLayers(), paint_rect,
                  geometry, bleed_avoidance);
}

void NGBoxFragmentPainter::PaintScrollHitTestDisplayItem(const PaintInfo&) {}

bool NGBoxFragmentPainter::NodeAtPoint(
    HitTestResult& result,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    HitTestAction action) {
  // TODO(eae): Switch to using NG geometry types.
  LayoutSize offset(box_fragment_.Offset().left, box_fragment_.Offset().top);
  LayoutPoint adjusted_location = accumulated_offset + offset;
  LayoutSize size(box_fragment_.Size().width, box_fragment_.Size().height);
  const ComputedStyle& style = box_fragment_.Style();

  bool hit_test_self = action == kHitTestForeground;

  // TODO(layout-dev): Add support for hit testing overflow controls once we
  // overflow has been implemented.
  // if (hit_test_self && HasOverflowClip() &&
  //   HitTestOverflowControl(result, location_in_container, adjusted_location))
  // return true;

  bool skip_children = false;
  if (box_fragment_.ShouldClipOverflow()) {
    // PaintLayer::HitTestContentsForFragments checked the fragments'
    // foreground rect for intersection if a layer is self painting,
    // so only do the overflow clip check here for non-self-painting layers.
    if (!box_fragment_.HasSelfPaintingLayer() &&
        !location_in_container.Intersects(box_fragment_.OverflowClipRect(
            adjusted_location, kExcludeOverlayScrollbarSizeForHitTesting))) {
      skip_children = true;
    }
    if (!skip_children && style.HasBorderRadius()) {
      LayoutRect bounds_rect(adjusted_location, size);
      skip_children = !location_in_container.Intersects(
          style.GetRoundedInnerBorderFor(bounds_rect));
    }
  }

  if (!skip_children &&
      HitTestChildren(result, box_fragment_.Children(), location_in_container,
                      adjusted_location, action)) {
    return true;
  }

  // TODO(eae): Implement once we support clipping in LayoutNG.
  // if (style.HasBorderRadius() &&
  //     HitTestClippedOutByBorder(location_in_container, adjusted_location))
  // return false;

  // Now hit test ourselves.
  if (hit_test_self && VisibleToHitTestRequest(result.GetHitTestRequest())) {
    LayoutRect bounds_rect(adjusted_location, size);
    if (location_in_container.Intersects(bounds_rect)) {
      Node* node = box_fragment_.GetNode();
      if (!result.InnerNode() && node) {
        LayoutPoint point =
            location_in_container.Point() - ToLayoutSize(adjusted_location);
        result.SetNodeAndPosition(node, point);
      }
      if (result.AddNodeToListBasedTestResult(node, location_in_container,
                                              bounds_rect) == kStopHitTesting) {
        return true;
      }
    }
  }

  return false;
}

bool NGBoxFragmentPainter::VisibleToHitTestRequest(
    const HitTestRequest& request) const {
  return box_fragment_.Style().Visibility() == EVisibility::kVisible &&
         (request.IgnorePointerEventsNone() ||
          box_fragment_.Style().PointerEvents() != EPointerEvents::kNone) &&
         !(box_fragment_.GetNode() && box_fragment_.GetNode()->IsInert());
}

bool NGBoxFragmentPainter::HitTestTextFragment(
    HitTestResult& result,
    const NGPhysicalFragment& text_fragment,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset) {
  LayoutSize offset(text_fragment.Offset().left, text_fragment.Offset().top);
  LayoutPoint adjusted_location = accumulated_offset + offset;
  LayoutSize size(text_fragment.Size().width, text_fragment.Size().height);
  LayoutRect border_rect(adjusted_location, size);
  const ComputedStyle& style = text_fragment.Style();

  if (style.HasBorderRadius()) {
    FloatRoundedRect border = style.GetRoundedBorderFor(
        border_rect,
        text_fragment.BorderEdges() & NGBorderEdges::Physical::kLeft,
        text_fragment.BorderEdges() & NGBorderEdges::Physical::kRight);
    if (!location_in_container.Intersects(border))
      return false;
  }

  // TODO(layout-dev): Clip to line-top/bottom.
  LayoutRect rect = LayoutRect(PixelSnappedIntRect(border_rect));
  if (VisibleToHitTestRequest(result.GetHitTestRequest()) &&
      location_in_container.Intersects(rect)) {
    Node* node = text_fragment.GetNode();
    if (!result.InnerNode() && node) {
      LayoutPoint point =
          location_in_container.Point() - ToLayoutSize(accumulated_offset);
      result.SetNodeAndPosition(node, point);
    }

    if (result.AddNodeToListBasedTestResult(node, location_in_container,
                                            rect) == kStopHitTesting) {
      return true;
    }
  }

  return false;
}

bool NGBoxFragmentPainter::HitTestChildren(
    HitTestResult& result,
    const Vector<std::unique_ptr<NGPaintFragment>>& children,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    HitTestAction action) {
  for (auto iter = children.rbegin(); iter != children.rend(); iter++) {
    const std::unique_ptr<NGPaintFragment>& child = *iter;

    // TODO(layout-dev): Handle self painting layers.
    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    bool stop_hit_testing = false;
    if (fragment.Type() == NGPhysicalFragment::kFragmentBox) {
      if (NGFragmentPainter::RequiresLegacyFallback(fragment)) {
        stop_hit_testing = fragment.GetLayoutObject()->NodeAtPoint(
            result, location_in_container, accumulated_offset, action);
      } else {
        stop_hit_testing = NGBoxFragmentPainter(*child).NodeAtPoint(
            result, location_in_container, accumulated_offset, action);
      }

    } else if (fragment.Type() == NGPhysicalFragment::kFragmentLineBox) {
      stop_hit_testing =
          HitTestChildren(result, child->Children(), location_in_container,
                          accumulated_offset, action);

    } else if (fragment.Type() == NGPhysicalFragment::kFragmentText) {
      // TODO(eae): Should this hit test on the text itself or the containing
      // node?
      stop_hit_testing = HitTestTextFragment(
          result, fragment, location_in_container, accumulated_offset);
    }
    if (stop_hit_testing)
      return true;
  }

  return false;
}

}  // namespace blink
