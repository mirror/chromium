// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ng_box_fragment_painter_h
#define ng_box_fragment_painter_h

#include "core/layout/BackgroundBleedAvoidance.h"
#include "core/layout/api/HitTestAction.h"
#include "core/layout/ng/geometry/ng_border_edges.h"
#include "core/paint/BoxPainterBase.h"
#include "platform/geometry/LayoutPoint.h"
#include "platform/geometry/LayoutSize.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class FillLayer;
class HitTestLocation;
class HitTestRequest;
class HitTestResult;
class Image;
class LayoutRect;
class NGPaintFragment;
class NGPhysicalFragment;
struct PaintInfo;

// Painter for LayoutNG box fragments, paints borders and background. Delegates
// to NGTextFragmentPainter to paint line box fragments.
class NGBoxFragmentPainter : public BoxPainterBase {
  STACK_ALLOCATED();

 public:
  NGBoxFragmentPainter(const NGPaintFragment&);

  void Paint(const PaintInfo&, const LayoutPoint&);
  void PaintInlineBox(const PaintInfo&,
                      const LayoutPoint&,
                      const LayoutPoint& block_paint_offset);

  // TODO(eae): Change to take a HitTestResult pointer instead as it mutates.
  bool NodeAtPoint(HitTestResult&,
                   const HitTestLocation& location_in_container,
                   const LayoutPoint& accumulated_offset,
                   HitTestAction);

 protected:
  BoxPainterBase::FillLayerInfo GetFillLayerInfo(
      const Color&,
      const FillLayer&,
      BackgroundBleedAvoidance) const override;

  void PaintFillLayerTextFillBox(GraphicsContext&,
                                 const BoxPainterBase::FillLayerInfo&,
                                 Image*,
                                 SkBlendMode composite_op,
                                 const BackgroundImageGeometry&,
                                 const LayoutRect&,
                                 LayoutRect scrolled_paint_rect) const override;
  LayoutRect AdjustForScrolledContent(const PaintInfo&,
                                      const BoxPainterBase::FillLayerInfo&,
                                      const LayoutRect&) const override;

 private:
  bool IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
      const NGPaintFragment&,
      const PaintInfo&) const;
  bool IntersectsPaintRect(const PaintInfo&, const LayoutPoint&) const;

  void PaintWithAdjustedOffset(PaintInfo&, const LayoutPoint&);
  void PaintBoxDecorationBackground(const PaintInfo&, const LayoutPoint&) const;
  void PaintAllPhasesAtomically(const PaintInfo&, const LayoutPoint&);
  void PaintChildren(const Vector<std::unique_ptr<NGPaintFragment>>&,
                     const PaintInfo&,
                     const LayoutPoint&);
  void PaintInlineChildren(const Vector<std::unique_ptr<NGPaintFragment>>&,
                           const PaintInfo&,
                           const LayoutPoint&);
  void PaintInlineChildrenOutlines(
      const Vector<std::unique_ptr<NGPaintFragment>>&,
      const PaintInfo&,
      const LayoutPoint& paint_offset) const;
  void PaintInlineChildBoxUsingLegacyFallback(const NGPhysicalFragment&,
                                              const PaintInfo&,
                                              const LayoutPoint&) const;
  void PaintText(const NGPaintFragment&,
                 const PaintInfo&,
                 const LayoutPoint& paint_offset);
  void PaintObject(const PaintInfo&, const LayoutPoint&);
  void PaintBlockFlowContents(const PaintInfo&, const LayoutPoint&);
  void PaintInlineChild(const NGPaintFragment&,
                        const PaintInfo&,
                        const LayoutPoint& paint_offset) const;
  void PaintInlineBlockChild(const NGPaintFragment&,
                             const PaintInfo&,
                             const LayoutPoint& paint_offset) const;
  void PaintTextChild(const NGPaintFragment&,
                      const PaintInfo&,
                      const LayoutPoint& paint_offset) const;
  void PaintContents(const PaintInfo&, const LayoutPoint&);
  void PaintFloats(const PaintInfo&, const LayoutPoint&);
  void PaintMask(const PaintInfo&, const LayoutPoint&) const;
  void PaintClippingMask(const PaintInfo&, const LayoutPoint&);
  void PaintOverflowControlsIfNeeded(const PaintInfo&, const LayoutPoint&);
  void PaintInlineBlock(const PaintInfo&, const LayoutPoint& paint_offset);
  void PaintLineBox(const NGPaintFragment&,
                    const PaintInfo&,
                    const LayoutPoint&);
  void PaintBackground(const PaintInfo&,
                       const LayoutRect&,
                       const Color& background_color,
                       BackgroundBleedAvoidance = kBackgroundBleedNone) const;

  bool VisibleToHitTestRequest(const HitTestRequest&) const;
  bool HitTestChildren(HitTestResult&,
                       const Vector<std::unique_ptr<NGPaintFragment>>&,
                       const HitTestLocation& location_in_container,
                       const LayoutPoint& accumulated_offset,
                       HitTestAction);
  bool HitTestTextFragment(HitTestResult&,
                           const NGPhysicalFragment&,
                           const HitTestLocation& location_in_container,
                           const LayoutPoint& accumulated_offset);

  const NGPaintFragment& box_fragment_;

  NGBorderEdges border_edges_;

  // True when this is an inline box.
  bool is_inline_;

  // The paint offset of the container block when painting inline children.
  LayoutPoint block_paint_offset_;
};

}  // namespace blink

#endif  // ng_box_fragment_painter_h
