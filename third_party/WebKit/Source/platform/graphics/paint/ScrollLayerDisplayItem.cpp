// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/paint/ScrollLayerDisplayItem.h"

#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/PaintController.h"
#include "platform/wtf/Assertions.h"

namespace blink {

ScrollLayerDisplayItem::ScrollLayerDisplayItem(const DisplayItemClient& client,
                                               Type type)
    : DisplayItem(client, type, sizeof(*this)) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  DCHECK(IsScrollLayerType(type));
}

ScrollLayerDisplayItem::~ScrollLayerDisplayItem() {}

void ScrollLayerDisplayItem::Replay(GraphicsContext&) const {
  NOTREACHED();
}

void ScrollLayerDisplayItem::AppendToWebDisplayItemList(
    const LayoutSize&,
    WebDisplayItemList*) const {
  NOTREACHED();
}

bool ScrollLayerDisplayItem::DrawsContent() const {
  return true;
}

bool ScrollLayerDisplayItem::Equals(const DisplayItem& other) const {
  return DisplayItem::Equals(other);
}

#ifndef NDEBUG
void ScrollLayerDisplayItem::DumpPropertiesAsDebugString(
    StringBuilder& string_builder) const {
  DisplayItem::DumpPropertiesAsDebugString(string_builder);
}
#endif  // NDEBUG

void RecordScrollLayer(GraphicsContext& context,
                       const DisplayItemClient& client,
                       DisplayItem::Type type) {
  PaintController& paint_controller = context.GetPaintController();
  if (paint_controller.DisplayItemConstructionIsDisabled())
    return;

  paint_controller.CreateAndAppend<ScrollLayerDisplayItem>(client, type);
}

}  // namespace blink
