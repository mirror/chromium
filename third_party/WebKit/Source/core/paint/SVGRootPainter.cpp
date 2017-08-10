// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/SVGRootPainter.h"

#include "core/layout/svg/LayoutSVGRoot.h"
#include "core/layout/svg/SVGLayoutSupport.h"

#include "core/layout/LayoutView.h"

#include "core/paint/BoxClipper.h"
#include "core/paint/BoxPainter.h"
#include "core/paint/ObjectPaintProperties.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/PaintTiming.h"
#include "core/paint/SVGPaintContext.h"
#include "core/svg/SVGSVGElement.h"
#include "platform/wtf/Optional.h"

#include <iostream>

/////// WO SVG investigation. DO NOT SUBMIT.
namespace {

static bool paint_called = false;

// static int count = 0;
// static int visible = 0;
// static int width = 0;

#if 0  // removed due to excess output, but kept for a potential later
       // refinement
static bool isVisible(blink::LayoutRect rect) {
  if (rect.IsEmpty())
    return false;
  if (rect.MaxX() < 0)  // left of screen
    return false;
  if (rect.MaxY() < 0)  // above screen
    return false;
  if (rect.X() > width)  // right of screen
    return false;
  // below is ok; we assume user could scroll there
  return true;
}
#endif

}  // namespace
///////// End WO

namespace blink {

IntRect SVGRootPainter::PixelSnappedSize(
    const LayoutPoint& paint_offset) const {
  return PixelSnappedIntRect(paint_offset, layout_svg_root_.Size());
}

AffineTransform SVGRootPainter::TransformToPixelSnappedBorderBox(
    const LayoutPoint& paint_offset) const {
  const IntRect snapped_size = PixelSnappedSize(paint_offset);
  AffineTransform paint_offset_to_border_box =
      AffineTransform::Translation(snapped_size.X(), snapped_size.Y());
  LayoutSize size = layout_svg_root_.Size();
  if (!size.IsEmpty()) {
    paint_offset_to_border_box.Scale(
        snapped_size.Width() / size.Width().ToFloat(),
        snapped_size.Height() / size.Height().ToFloat());
  }
  paint_offset_to_border_box.Multiply(
      layout_svg_root_.LocalToBorderBoxTransform());
  return paint_offset_to_border_box;
}

void SVGRootPainter::PaintReplaced(const PaintInfo& paint_info,
                                   const LayoutPoint& paint_offset) {
  // count++;
  // std::cout << ">>>WO_SVG: " << count << std::endl;

  if (!paint_called) {
    std::cout << ">>>WO_SVG: SVGRootPainter::PaintReplaced called" << std::endl;
    paint_called = true;
  }

  // An empty viewport disables rendering.
  if (PixelSnappedSize(paint_offset).IsEmpty())
    return;

  // An empty viewBox also disables rendering.
  // (http://www.w3.org/TR/SVG/coords.html#ViewBoxAttribute)
  SVGSVGElement* svg = toSVGSVGElement(layout_svg_root_.GetNode());
  DCHECK(svg);
  if (svg->HasEmptyViewBox())
    return;

  // Apply initial viewport clip.
  Optional<BoxClipper> box_clipper;
  if (layout_svg_root_.ShouldApplyViewportClip()) {
    // TODO(pdr): Clip the paint info cull rect here.
    box_clipper.emplace(layout_svg_root_, paint_info, paint_offset,
                        kForceContentsClip);
  }

  PaintInfo paint_info_before_filtering(paint_info);
  AffineTransform transform_to_border_box =
      TransformToPixelSnappedBorderBox(paint_offset);
  paint_info_before_filtering.UpdateCullRect(transform_to_border_box);
  SVGTransformContext transform_context(paint_info_before_filtering.context,
                                        layout_svg_root_,
                                        transform_to_border_box);

  SVGPaintContext paint_context(layout_svg_root_, paint_info_before_filtering);
  if (paint_context.GetPaintInfo().phase == kPaintPhaseForeground &&
      !paint_context.ApplyClipMaskAndFilterIfNecessary())
    return;

/////// WO SVG investigation. DO NOT SUBMIT.
#if 0  // removed due to excess output, but kept for a potential later
       // refinement
  LayoutBox const& box =
      layout_svg_root_.GetDocument().GetLayoutView()->RootBox();
  LayoutRect framerect = box.FrameRect();
  // std::cout << "root box rect: " << framerect.ToString() << std::endl;
  int nwidth = framerect.Width().ToInt();
  if (nwidth != width) {
    width = nwidth;
  }

  LayoutRect rect = layout_svg_root_.VisualRect();
  LayoutRect prerect = rect;
  layout_svg_root_.MapToVisualRectInAncestorSpace(&box, rect);
  if (isVisible(rect)) {
    visible++;
    std::cout << ">>>WO_SVG visible paint (" << visible << " of " << count
              << " paint calls, " << visible * 100 / count << "%) mapped "
              << prerect.ToString() << " to: " << rect.ToString()
              << " on width " << width << std::endl;
  }
#endif
  ///////// End WO

  BoxPainter(layout_svg_root_)
      .PaintChildren(paint_context.GetPaintInfo(), LayoutPoint());

  PaintTiming& timing = PaintTiming::From(
      layout_svg_root_.GetNode()->GetDocument().TopDocument());
  timing.MarkFirstContentfulPaint();
}

}  // namespace blink
