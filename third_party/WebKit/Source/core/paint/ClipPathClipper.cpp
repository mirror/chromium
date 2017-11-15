// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ClipPathClipper.h"

#include "core/dom/ElementTraversal.h"
#include "core/layout/svg/LayoutSVGResourceClipper.h"
#include "core/layout/svg/SVGLayoutSupport.h"
#include "core/layout/svg/SVGResources.h"
#include "core/layout/svg/SVGResourcesCache.h"
#include "core/paint/PaintInfo.h"
#include "core/style/ClipPathOperation.h"
#include "platform/graphics/paint/DrawingDisplayItem.h"
#include "platform/graphics/paint/DrawingRecorder.h"
#include "platform/graphics/paint/PaintController.h"
#include "platform/graphics/paint/PaintRecordBuilder.h"

namespace blink {

namespace {

class SVGClipExpansionCycleHelper {
 public:
  void Lock(LayoutSVGResourceClipper& clipper) {
    DCHECK(!clipper.HasCycle());
    clipper.BeginClipExpansion();
    clippers_.push_back(&clipper);
  }
  ~SVGClipExpansionCycleHelper() {
    for (auto* clipper : clippers_)
      clipper->EndClipExpansion();
  }

 private:
  Vector<LayoutSVGResourceClipper*, 1> clippers_;
};

LayoutSVGResourceClipper* ResolveElementReference(
    const LayoutObject& layout_object,
    const ReferenceClipPathOperation& reference_clip_path_operation) {
  if (layout_object.IsSVGChild()) {
    // The reference will have been resolved in
    // SVGResources::buildResources, so we can just use the LayoutObject's
    // SVGResources.
    SVGResources* resources =
        SVGResourcesCache::CachedResourcesForLayoutObject(&layout_object);
    return resources ? resources->Clipper() : nullptr;
  }
  // TODO(fs): Doesn't work with external SVG references (crbug.com/109212.)
  Node* target_node = layout_object.GetNode();
  if (!target_node)
    return nullptr;
  SVGElement* element =
      reference_clip_path_operation.FindElement(target_node->GetTreeScope());
  if (!IsSVGClipPathElement(element) || !element->GetLayoutObject())
    return nullptr;
  return ToLayoutSVGResourceClipper(
      ToLayoutSVGResourceContainer(element->GetLayoutObject()));
}

}  // namespace

static bool DegenerateToRect(const ClipPathOperation&) {
  // TODO(trchen): Implement!
  return false;
}

ClipPathClipper::ClipPathClipper(GraphicsContext& context,
                                 const LayoutObject& layout_object,
                                 const LayoutPoint& paint_offset)
    : context_(context),
      layout_object_(layout_object),
      paint_offset_(paint_offset) {
  DCHECK(layout_object_.StyleRef().ClipPath());

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Optional<FloatRect> bounding_box = layout_object_.LocalClipPathBoundingBox();
  if (!bounding_box)
    return;

  FloatRect adjusted_bounding_box = *bounding_box;
  adjusted_bounding_box.MoveBy(FloatPoint(paint_offset_));
  clip_recorder_.emplace(context_, layout_object_,
                         DisplayItem::kFloatClipClipPathBounds,
                         adjusted_bounding_box);

  const ClipPathOperation& clip_path = *layout_object_.StyleRef().ClipPath();
  degenerate_to_rect_ = DegenerateToRect(clip_path);
  if (degenerate_to_rect_)
    return;
  mask_isolation_recorder_.emplace(context_, layout_object_,
                                   SkBlendMode::kSrcOver, 1.f);
}

// Note: Return resolved LayoutSVGResourceClipper for caller's convenience,
// if the clip path is a reference to SVG.
static bool IsClipPathOperationValid(
    const ClipPathOperation& clip_path,
    const LayoutObject& search_scope,
    LayoutSVGResourceClipper*& resource_clipper) {
  if (clip_path.GetType() == ClipPathOperation::SHAPE) {
    if (!ToShapeClipPathOperation(clip_path).IsValid())
      return false;
  } else {
    DCHECK_EQ(clip_path.GetType(), ClipPathOperation::REFERENCE);
    resource_clipper = ResolveElementReference(
        search_scope, ToReferenceClipPathOperation(clip_path));
    if (!resource_clipper)
      return false;
    SECURITY_DCHECK(!resource_clipper->NeedsLayout());
    resource_clipper->ClearInvalidationMask();
    if (resource_clipper->HasCycle())
      return false;
  }
  return true;
}

ClipPathClipper::~ClipPathClipper() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  if (degenerate_to_rect_)
    return;

  FloatRect reference_box = layout_object_.LocalReferenceBoxForClipPath();
  // The recorders have to live through the clip path chain, but we want to
  // create them only if at least one clip path in the chain is valid.
  Optional<CompositingRecorder> mask_recorder;
  Optional<DrawingRecorder> recorder;
  SVGClipExpansionCycleHelper locks;
  bool is_first = true;
  const LayoutObject* current_object = &layout_object_;
  while (current_object) {
    const ClipPathOperation* clip_path = current_object->StyleRef().ClipPath();
    if (!clip_path)
      break;
    LayoutSVGResourceClipper* resource_clipper = nullptr;
    if (!IsClipPathOperationValid(*clip_path, *current_object,
                                  resource_clipper))
      break;
    if (is_first) {
      mask_recorder.emplace(context_, layout_object_, SkBlendMode::kDstIn, 1.f);
      if (DrawingRecorder::UseCachedDrawingIfPossible(context_, layout_object_,
                                                      DisplayItem::kSVGClip))
        return;
      recorder.emplace(context_, layout_object_, DisplayItem::kSVGClip);
      context_.Save();
      context_.Translate(paint_offset_.X(), paint_offset_.Y());
    } else {
      context_.BeginLayer(1.f, SkBlendMode::kDstIn);
    }

    if (resource_clipper) {
      DCHECK_EQ(clip_path->GetType(), ClipPathOperation::REFERENCE);
      locks.Lock(*resource_clipper);
      AffineTransform mask_to_content;
      mask_to_content.Multiply(
          ToSVGClipPathElement(resource_clipper->GetElement())
              ->CalculateTransform(SVGElement::kIncludeMotionTransform));
      if (resource_clipper->ClipPathUnits() ==
          SVGUnitTypes::kSvgUnitTypeUserspaceonuse) {
        if (!layout_object_.IsSVG())
          mask_to_content.Scale(resource_clipper->StyleRef().EffectiveZoom());
      } else {
        DCHECK_EQ(resource_clipper->ClipPathUnits(),
                  SVGUnitTypes::kSvgUnitTypeObjectboundingbox);
        mask_to_content.Translate(reference_box.X(), reference_box.Y());
        mask_to_content.ScaleNonUniform(reference_box.Width(),
                                        reference_box.Height());
      }
      context_.Save();
      context_.ConcatCTM(mask_to_content);
      context_.DrawRecord(resource_clipper->CreatePaintRecord());
      context_.Restore();
    } else {
      DCHECK_EQ(clip_path->GetType(), ClipPathOperation::SHAPE);
      auto& shape = ToShapeClipPathOperation(*clip_path);
      PaintFlags flags;
      flags.setColor(Color::kBlack);
      flags.setAntiAlias(true);
      context_.DrawPath(shape.GetPath(reference_box)->GetSkPath(), flags);
    }

    if (!is_first)
      context_.EndLayer();

    is_first = false;
    current_object = resource_clipper;
  }
  if (recorder)
    context_.Restore();
}

}  // namespace blink
