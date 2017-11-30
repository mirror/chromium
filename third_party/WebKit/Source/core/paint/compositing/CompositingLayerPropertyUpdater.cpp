// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/compositing/CompositingLayerPropertyUpdater.h"

#include "core/layout/LayoutBoxModelObject.h"
#include "core/paint/FragmentData.h"
#include "core/paint/compositing/CompositedLayerMapping.h"
#include "platform/runtime_enabled_features.h"

namespace blink {

void CompositingLayerPropertyUpdater::Update(const LayoutObject& object) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
      RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  if (!object.HasLayer())
    return;
  CompositedLayerMapping* mapping =
      ToLayoutBoxModelObject(object).Layer()->GetCompositedLayerMapping();
  if (!mapping)
    return;

  const FragmentData& fragment_data = object.FirstFragment();
  const RarePaintData* rare_paint_data = fragment_data.GetRarePaintData();
  DCHECK(rare_paint_data);
  DCHECK(rare_paint_data->LocalBorderBoxProperties());
  // SPv1 compositing forces single fragment for composited elements.
  DCHECK(!fragment_data.NextFragment());

  LayoutPoint layout_snapped_paint_offset =
      fragment_data.PaintOffset() - mapping->SubpixelAccumulation();
  IntPoint snapped_paint_offset = RoundedIntPoint(layout_snapped_paint_offset);
  DCHECK(layout_snapped_paint_offset == snapped_paint_offset);

  auto SetContainerLayerState =
      [rare_paint_data, &snapped_paint_offset](GraphicsLayer* graphics_layer) {
        if (graphics_layer) {
          graphics_layer->SetLayerState(
              PropertyTreeState(*rare_paint_data->LocalBorderBoxProperties()),
              snapped_paint_offset + graphics_layer->OffsetFromLayoutObject());
        }
      };
  SetContainerLayerState(mapping->MainGraphicsLayer());
  SetContainerLayerState(mapping->LayerForHorizontalScrollbar());
  SetContainerLayerState(mapping->LayerForVerticalScrollbar());
  SetContainerLayerState(mapping->LayerForScrollCorner());

  auto SetContentsLayerState =
      [rare_paint_data, &snapped_paint_offset](GraphicsLayer* graphics_layer) {
        if (graphics_layer) {
          graphics_layer->SetLayerState(
              rare_paint_data->ContentsProperties(),
              snapped_paint_offset + graphics_layer->OffsetFromLayoutObject());
        }
      };
  SetContentsLayerState(mapping->ScrollingContentsLayer());
  SetContentsLayerState(mapping->ForegroundLayer());

  if (GraphicsLayer* squashing_layer = mapping->SquashingLayer()) {
    squashing_layer->SetLayerState(
        rare_paint_data->PreEffectProperties(),
        snapped_paint_offset + mapping->SquashingLayerOffsetFromLayoutObject());
  }

  if (auto* child_clipping_mask_layer = mapping->ChildClippingMaskLayer()) {
    child_clipping_mask_layer->SetLayerState(
        rare_paint_data->PreEffectProperties(),
        snapped_paint_offset +
            child_clipping_mask_layer->OffsetFromLayoutObject());
  }

  if (auto* ancestor_clipping_mask_layer =
          mapping->AncestorClippingMaskLayer()) {
    // AncestorClippingMaskLayer's state is in |clip_inheritance_ancestor|.
    const auto* compositing_container =
        mapping->OwningLayer().EnclosingLayerWithCompositedLayerMapping(
            kExcludeSelf);
    const auto* clip_inheritance_ancestor =
        mapping->ClipInheritanceAncestor(compositing_container);
    DCHECK(clip_inheritance_ancestor);
    const auto* clip_inheritance_ancestor_graphics_layer =
        clip_inheritance_ancestor->GraphicsLayerBacking();
    DCHECK(clip_inheritance_ancestor_graphics_layer);

    // Also calculate the the layer's offset from |clip_inheritance_ancestor|.
    IntPoint offset_from_clip_ancestor;
    for (const auto* parent = mapping->AncestorClippingLayer();
         parent != clip_inheritance_ancestor_graphics_layer;
         parent = parent->Parent()) {
      DCHECK(parent);
      // TODO(wangxianzhu): What about subpixel positions?
      offset_from_clip_ancestor.MoveBy(RoundedIntPoint(parent->GetPosition()));
    }

    ancestor_clipping_mask_layer->SetLayerState(
        PropertyTreeState(*clip_inheritance_ancestor->GetLayoutObject()
                               .FirstFragment()
                               .LocalBorderBoxProperties()),
        offset_from_clip_ancestor);
  }

  // TODO(crbug.com/790548): Complete for all drawable layers.
}

void CompositingLayerPropertyUpdater::Update(const LocalFrameView& frame_view) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
      RuntimeEnabledFeatures::SlimmingPaintV2Enabled() ||
      RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return;

  auto SetOverflowControlLayerState =
      [&frame_view](GraphicsLayer* graphics_layer) {
        if (graphics_layer) {
          graphics_layer->SetLayerState(
              frame_view.PreContentClipProperties(),
              IntPoint(graphics_layer->OffsetFromLayoutObject()));
        }
      };
  SetOverflowControlLayerState(frame_view.LayerForHorizontalScrollbar());
  SetOverflowControlLayerState(frame_view.LayerForVerticalScrollbar());
  SetOverflowControlLayerState(frame_view.LayerForScrollCorner());
}

}  // namespace blink
