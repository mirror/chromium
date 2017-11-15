/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2011 Dirk Schulze <krit@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/layout/svg/LayoutSVGResourceClipper.h"

#include "core/dom/ElementTraversal.h"
#include "core/layout/HitTestResult.h"
#include "core/layout/svg/SVGLayoutSupport.h"
#include "core/paint/PaintInfo.h"
#include "core/svg/SVGGeometryElement.h"
#include "core/svg/SVGUseElement.h"
#include "platform/graphics/paint/PaintRecord.h"
#include "platform/graphics/paint/PaintRecordBuilder.h"
#include "third_party/skia/include/pathops/SkPathOps.h"

namespace blink {

static bool ContributesToClip(const SVGElement& element) {
  // <use> within <clipPath> have a restricted content model.
  // (https://drafts.fxtf.org/css-masking-1/#ClipPathElement)
  if (IsSVGUseElement(element)) {
    const LayoutObject* use_layout_object = element.GetLayoutObject();
    if (!use_layout_object ||
        use_layout_object->StyleRef().Display() == EDisplay::kNone)
      return false;
    const SVGGraphicsElement* shape_element =
        ToSVGUseElement(element).VisibleTargetGraphicsElementForClipping();
    if (!shape_element)
      return false;
    return ContributesToClip(*shape_element);
  }
  if (!element.IsSVGGraphicsElement())
    return false;

  const LayoutObject* layout_object = element.GetLayoutObject();
  if (!layout_object)
    return false;
  const ComputedStyle& style = layout_object->StyleRef();
  if (style.Display() == EDisplay::kNone ||
      style.Visibility() != EVisibility::kVisible)
    return false;
  return layout_object->IsSVGShape() || layout_object->IsSVGText();
}

LayoutSVGResourceClipper::LayoutSVGResourceClipper(SVGClipPathElement* node)
    : LayoutSVGResourceContainer(node), in_clip_expansion_(false) {}

LayoutSVGResourceClipper::~LayoutSVGResourceClipper() {}

void LayoutSVGResourceClipper::RemoveAllClientsFromCache(
    bool mark_for_invalidation) {
  cached_paint_record_.reset();
  local_clip_bounds_ = FloatRect();
  MarkAllClientsForInvalidation(mark_for_invalidation
                                    ? kLayoutAndBoundariesInvalidation
                                    : kParentOnlyInvalidation);
}

void LayoutSVGResourceClipper::RemoveClientFromCache(
    LayoutObject* client,
    bool mark_for_invalidation) {
  DCHECK(client);
  MarkClientForInvalidation(client, mark_for_invalidation
                                        ? kBoundariesInvalidation
                                        : kParentOnlyInvalidation);
}

sk_sp<const PaintRecord> LayoutSVGResourceClipper::CreatePaintRecord() {
  DCHECK(GetFrame());
  if (cached_paint_record_)
    return cached_paint_record_;

  PaintRecordBuilder builder(nullptr, nullptr);
  // Switch to a paint behavior where all children of this <clipPath> will be
  // laid out using special constraints:
  // - fill-opacity/stroke-opacity/opacity set to 1
  // - masker/filter not applied when laying out the children
  // - fill is set to the initial fill paint server (solid, black)
  // - stroke is set to the initial stroke paint server (none)
  PaintInfo info(builder.Context(), LayoutRect::InfiniteIntRect(),
                 PaintPhase::kForeground, kGlobalPaintNormalPhase,
                 kPaintLayerPaintingRenderingClipPathAsMask |
                     kPaintLayerPaintingRenderingResourceSubtree);

  for (const SVGElement& child_element :
       Traversal<SVGElement>::ChildrenOf(*GetElement())) {
    if (!ContributesToClip(child_element))
      continue;
    // Use the LayoutObject of the direct child even if it is a <use>. In that
    // case, we will paint the targeted element indirectly.
    const LayoutObject* layout_object = child_element.GetLayoutObject();
    layout_object->Paint(info, IntPoint());
  }

  cached_paint_record_ = builder.EndRecording();
  return cached_paint_record_;
}

void LayoutSVGResourceClipper::CalculateLocalClipBounds() {
  // This is a rough heuristic to appraise the clip size and doesn't consider
  // clip on clip.
  for (const SVGElement& child_element :
       Traversal<SVGElement>::ChildrenOf(*GetElement())) {
    if (!ContributesToClip(child_element))
      continue;
    const LayoutObject* layout_object = child_element.GetLayoutObject();
    local_clip_bounds_.Unite(layout_object->LocalToSVGParentTransform().MapRect(
        layout_object->VisualRectInLocalSVGCoordinates()));
  }
}

bool LayoutSVGResourceClipper::HitTestClipContent(
    const FloatRect& object_bounding_box,
    const FloatPoint& node_at_point) {
  FloatPoint point = node_at_point;
  if (!SVGLayoutSupport::PointInClippingArea(*this, point))
    return false;

  if (ClipPathUnits() == SVGUnitTypes::kSvgUnitTypeObjectboundingbox) {
    AffineTransform transform;
    transform.Translate(object_bounding_box.X(), object_bounding_box.Y());
    transform.ScaleNonUniform(object_bounding_box.Width(),
                              object_bounding_box.Height());
    point = transform.Inverse().MapPoint(point);
  }

  AffineTransform animated_local_transform =
      ToSVGClipPathElement(GetElement())
          ->CalculateTransform(SVGElement::kIncludeMotionTransform);
  if (!animated_local_transform.IsInvertible())
    return false;

  point = animated_local_transform.Inverse().MapPoint(point);

  for (const SVGElement& child_element :
       Traversal<SVGElement>::ChildrenOf(*GetElement())) {
    if (!ContributesToClip(child_element))
      continue;
    IntPoint hit_point;
    HitTestResult result(HitTestRequest::kSVGClipContent, hit_point);
    LayoutObject* layout_object = child_element.GetLayoutObject();
    if (layout_object->NodeAtFloatPoint(result, point, kHitTestForeground))
      return true;
  }
  return false;
}

FloatRect LayoutSVGResourceClipper::ResourceBoundingBox(
    const FloatRect& reference_box) {
  // The resource has not been layouted yet. Return the reference box.
  if (SelfNeedsLayout())
    return reference_box;

  if (local_clip_bounds_.IsEmpty())
    CalculateLocalClipBounds();

  AffineTransform transform =
      ToSVGClipPathElement(GetElement())
          ->CalculateTransform(SVGElement::kIncludeMotionTransform);
  if (ClipPathUnits() == SVGUnitTypes::kSvgUnitTypeObjectboundingbox) {
    transform.Translate(reference_box.X(), reference_box.Y());
    transform.ScaleNonUniform(reference_box.Width(), reference_box.Height());
  }
  return transform.MapRect(local_clip_bounds_);
}

}  // namespace blink
