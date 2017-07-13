// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ScrollRecorder.h"

#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/PaintController.h"
#include "platform/graphics/paint/ScrollDisplayItem.h"
#include "platform/graphics/paint/ScrollLayerDisplayItem.h"

namespace blink {

// TODO(pdr): Ensure all ScrollRecorder uses are not guarded by SPV2 check.
// TODO(pdr): Make current offset optional.
ScrollRecorder::ScrollRecorder(GraphicsContext& context,
                               const DisplayItemClient& client,
                               DisplayItem::Type type,
                               const IntSize& current_offset)
    : client_(client), begin_item_type_(type), context_(context) {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    // TODO(pdr): Comment this.
    // TODO(pdr): Should we pass a size?
    RecordScrollLayer(context, client_, DisplayItem::kScrollLayer);
    return;
  }
  context_.GetPaintController().CreateAndAppend<BeginScrollDisplayItem>(
      client_, begin_item_type_, current_offset);
}

ScrollRecorder::ScrollRecorder(GraphicsContext& context,
                               const DisplayItemClient& client,
                               PaintPhase phase,
                               const IntSize& current_offset)
    : ScrollRecorder(context,
                     client,
                     DisplayItem::PaintPhaseToScrollType(phase),
                     current_offset) {}

ScrollRecorder::~ScrollRecorder() {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;
  context_.GetPaintController().EndItem<EndScrollDisplayItem>(
      client_, DisplayItem::ScrollTypeToEndScrollType(begin_item_type_));
}

}  // namespace blink
