// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/RarePaintData.h"

#include "core/paint/PaintLayer.h"

namespace blink {

ObjectPaintProperties& RarePaintData::EnsurePaintProperties() {
  if (!paint_properties_)
    paint_properties_ = ObjectPaintProperties::Create();
  return *paint_properties_.get();
}

void RarePaintData::ClearPaintProperties() {
  paint_properties_.reset(nullptr);
}

void RarePaintData::ClearLocalBorderBoxProperties() {
  local_border_box_properties_ = nullptr;
}

void RarePaintData::SetLocalBorderBoxProperties(PropertyTreeState& state) {
  if (!local_border_box_properties_)
    local_border_box_properties_ = WTF::MakeUnique<PropertyTreeState>(state);
  else
    *local_border_box_properties_ = state;
}

PropertyTreeState RarePaintData::ContentsProperties() const {
  DCHECK(local_border_box_properties_);
  PropertyTreeState contents(*local_border_box_properties_);
  if (auto* properties = PaintProperties()) {
    if (properties->ScrollTranslation())
      contents.SetTransform(properties->ScrollTranslation());
    if (properties->OverflowClip())
      contents.SetClip(properties->OverflowClip());
    else if (properties->CssClip())
      contents.SetClip(properties->CssClip());
  }

  // TODO(chrishtr): cssClipFixedPosition needs to be handled somehow.

  return contents;
}

RarePaintData::RarePaintData(const LayoutPoint& location_in_backing)
    : unique_id_(NewUniqueObjectId()),
      location_in_backing_(location_in_backing) {}

RarePaintData::~RarePaintData() {}

void RarePaintData::SetLayer(std::unique_ptr<PaintLayer> layer) {
  layer_ = std::move(layer);
};

}  // namespace blink
