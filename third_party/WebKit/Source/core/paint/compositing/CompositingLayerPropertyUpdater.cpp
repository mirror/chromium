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
  SetContainerLayerState(mapping->DecorationOutlineLayer());
  SetContainerLayerState(mapping->BackgroundLayer());

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

  if (auto* squashing_layer = mapping->SquashingLayer()) {
    squashing_layer->SetLayerState(
        rare_paint_data->PreEffectProperties(),
        snapped_paint_offset + mapping->SquashingLayerOffsetFromLayoutObject());
  }

  if (auto* mask_layer = mapping->MaskLayer()) {
    auto state = *rare_paint_data->LocalBorderBoxProperties();
    const auto* properties = rare_paint_data->PaintProperties();
    DCHECK(properties && properties->Mask());
    state.SetEffect(properties->Mask());
    mask_layer->SetLayerState(
        std::move(state),
        snapped_paint_offset + mask_layer->OffsetFromLayoutObject());
  }

  if (auto* child_clipping_mask_layer = mapping->ChildClippingMaskLayer()) {
    child_clipping_mask_layer->SetLayerState(
        rare_paint_data->PreEffectProperties(),
        snapped_paint_offset +
            child_clipping_mask_layer->OffsetFromLayoutObject());
  }

  if (auto* ancestor_clipping_mask_layer =
          mapping->AncestorClippingMaskLayer()) {
    // AncestorClippingMaskLayer's clip and transform states (for applying
    // border radius cliping on descendants) are is in
    // |clip_inheritance_ancestor|.
    const auto* clip_inheritance_ancestor = mapping->ClipInheritanceAncestor(
        mapping->OwningLayer().EnclosingLayerWithCompositedLayerMapping(
            kExcludeSelf));
    DCHECK(clip_inheritance_ancestor);
    auto state = *clip_inheritance_ancestor->GetLayoutObject()
                      .FirstFragment()
                      .LocalBorderBoxProperties();
    state.SetEffect(rare_paint_data->PreEffectProperties().Effect());
    // Also convert layer offset into |clip_inheritance_ancestor| coordinates.
    LayoutPoint layer_offset(
        snapped_paint_offset +
        ancestor_clipping_mask_layer->OffsetFromLayoutObject());
    mapping->OwningLayer().ConvertToLayerCoords(clip_inheritance_ancestor,
                                                layer_offset);
    ancestor_clipping_mask_layer->SetLayerState(std::move(state),
                                                RoundedIntPoint(layer_offset));
  }
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
