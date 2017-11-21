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

  if (GraphicsLayer* main_layer = mapping->MainGraphicsLayer()) {
    main_layer->SetLayerState(
        PropertyTreeState(*rare_paint_data->LocalBorderBoxProperties()),
        snapped_paint_offset + main_layer->OffsetFromLayoutObject());
  }
  if (GraphicsLayer* squashing_layer = mapping->SquashingLayer()) {
    squashing_layer->SetLayerState(
        rare_paint_data->PreEffectProperties(),
        snapped_paint_offset + mapping->SquashingLayerOffsetFromLayoutObject());
  }

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
  SetContentsLayerState(mapping->LayerForScrollCorner());

  if (const auto* scrollable_area =
          mapping->OwningLayer().GetScrollableArea()) {
    if (auto* horizontal_scrollbar_layer =
            mapping->LayerForHorizontalScrollbar()) {
      horizontal_scrollbar_layer->SetLayerState(
          PropertyTreeState(*rare_paint_data->LocalBorderBoxProperties()),
          snapped_paint_offset +
              horizontal_scrollbar_layer->OffsetFromLayoutObject() +
              scrollable_area->HorizontalScrollbar()->FrameRect().Location());
    }
    if (auto* vertical_scrollbar_layer = mapping->LayerForVerticalScrollbar()) {
      vertical_scrollbar_layer->SetLayerState(
          PropertyTreeState(*rare_paint_data->LocalBorderBoxProperties()),
          snapped_paint_offset +
              vertical_scrollbar_layer->OffsetFromLayoutObject() +
              scrollable_area->VerticalScrollbar()->FrameRect().Location());
    }
  }

  // TODO(trchen): Complete for all drawable layers.
}

void CompositingLayerPropertyUpdater::Update(const LocalFrameView& frame_view) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
      RuntimeEnabledFeatures::SlimmingPaintV2Enabled() ||
      RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return;

  const auto* contents_state = frame_view.TotalPropertyTreeStateForContents();
  if (!contents_state)
    return;

  auto* horizontal_scrollbar_layer = frame_view.LayerForHorizontalScrollbar();
  auto* vertical_scrollbar_layer = frame_view.LayerForVerticalScrollbar();
  auto* scroll_corner_layer = frame_view.LayerForScrollCorner();
  if (horizontal_scrollbar_layer || vertical_scrollbar_layer ||
      scroll_corner_layer) {
    PropertyTreeState scrollbar_state(frame_view.PreTranslation(),
                                      contents_state->Clip()->Parent(),
                                      contents_state->Effect());
    if (horizontal_scrollbar_layer) {
      horizontal_scrollbar_layer->SetLayerState(
          PropertyTreeState(scrollbar_state),
          frame_view.HorizontalScrollbar()->FrameRect().Location());
    }
    if (vertical_scrollbar_layer) {
      vertical_scrollbar_layer->SetLayerState(
          PropertyTreeState(scrollbar_state),
          frame_view.VerticalScrollbar()->FrameRect().Location());
    }
    if (scroll_corner_layer) {
      scroll_corner_layer->SetLayerState(PropertyTreeState(scrollbar_state),
                                         IntPoint());
    }
  }
  // TODO(trchen): Complete for all drawable layers.
}

}  // namespace blink
