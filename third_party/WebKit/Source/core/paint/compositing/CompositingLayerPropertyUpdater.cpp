// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/compositing/CompositingLayerPropertyUpdater.h"

#include "core/layout/LayoutBoxModelObject.h"
#include "core/paint/FragmentData.h"
#include "core/paint/compositing/CompositedLayerMapping.h"
#include "platform/runtime_enabled_features.h"

namespace blink {

static const TransformPaintPropertyNode* GetPreTransform(
    const PropertyTreeState& border_box_properties,
    const ObjectPaintProperties* properties) {
  if (!properties)
    return border_box_properties.Transform();
  if (properties->Transform())
    return properties->Transform()->Parent();
  return border_box_properties.Transform();
}

static const EffectPaintPropertyNode* GetPreEffect(
    const PropertyTreeState& border_box_properties,
    const ObjectPaintProperties* properties) {
  if (!properties)
    return border_box_properties.Effect();
  if (properties->Effect())
    return properties->Effect()->Parent();
  if (properties->Filter())
    return properties->Filter()->Parent();
  return border_box_properties.Effect();
}

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

  const FragmentData* fragment_data = object.FirstFragment();
  DCHECK(fragment_data);
  // SPv1 compositing forces single fragment for composited elements.
  DCHECK(!fragment_data->NextFragment());

  const ObjectPaintProperties* properties = fragment_data->PaintProperties();
  DCHECK(fragment_data->LocalBorderBoxProperties());
  const PropertyTreeState& box_state =
      *fragment_data->LocalBorderBoxProperties();
  LayoutPoint layout_snapped_paint_offset =
      fragment_data->PaintOffset() - mapping->SubpixelAccumulation();
  IntPoint snapped_paint_offset = RoundedIntPoint(layout_snapped_paint_offset);
  DCHECK(layout_snapped_paint_offset == snapped_paint_offset);

  if (GraphicsLayer* main_layer = mapping->MainGraphicsLayer()) {
    PropertyTreeState layer_state(box_state);
    IntPoint layer_offset =
        snapped_paint_offset + main_layer->OffsetFromLayoutObject();
    main_layer->SetLayerState(std::move(layer_state), layer_offset);
  }
  if (GraphicsLayer* scrolling_contents_layer =
          mapping->ScrollingContentsLayer()) {
    PropertyTreeState layer_state(properties->ScrollTranslation(),
                                  properties->OverflowClip(),
                                  box_state.Effect());
    IntPoint layer_offset = snapped_paint_offset +
                            scrolling_contents_layer->OffsetFromLayoutObject();
    scrolling_contents_layer->SetLayerState(std::move(layer_state),
                                            layer_offset);
  }
  if (GraphicsLayer* squashing_layer = mapping->SquashingLayer()) {
    PropertyTreeState layer_state(GetPreTransform(box_state, properties),
                                  box_state.Clip(),
                                  GetPreEffect(box_state, properties));
    IntPoint layer_offset =
        snapped_paint_offset + squashing_layer->OffsetFromLayoutObject();
    squashing_layer->SetLayerState(std::move(layer_state), layer_offset);
  }
  // TODO(trchen): Complete for all drawable layers.
}

}  // namespace blink
