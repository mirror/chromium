// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScopedPaintChunkProperties_h
#define ScopedPaintChunkProperties_h

#include "platform/graphics/paint/DisplayItem.h"
#include "platform/graphics/paint/PaintChunk.h"
#include "platform/graphics/paint/PaintChunkProperties.h"
#include "platform/graphics/paint/PaintController.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class ScopedPaintChunkProperties {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
  WTF_MAKE_NONCOPYABLE(ScopedPaintChunkProperties);

 public:
  ScopedPaintChunkProperties(PaintController& paint_controller,
                             const DisplayItemClient& client,
                             PaintChunk::Type type,
                             const PaintChunkProperties& properties)
      : paint_controller_(paint_controller),
        id_(client, type),
        previous_properties_(paint_controller.CurrentPaintChunkProperties()) {
    paint_controller_.UpdateCurrentPaintChunkProperties(&id_, properties);
  }

  ScopedPaintChunkProperties(PaintController& paint_controller,
                             const DisplayItemClient& client,
                             DisplayItem::Type type,
                             const PaintChunkProperties& properties)
      : ScopedPaintChunkProperties(paint_controller,
                                   client,
                                   static_cast<PaintChunk::Type>(type),
                                   properties) {}

  // Omits the type parameter, in case that the client creates only one
  // PaintChunkProperties node during each painting.
  ScopedPaintChunkProperties(PaintController& paint_controller,
                             const DisplayItemClient& client,
                             const PaintChunkProperties& properties)
      : ScopedPaintChunkProperties(paint_controller,
                                   client,
                                   DisplayItem::kUninitializedType,
                                   properties) {}

  ~ScopedPaintChunkProperties() {
    // We should not return to the previous id, because that may cause a new
    // chunk to use the same id as that of the previous chunk before this
    // ScopedPaintChunkProperties. The painter should create another scope of
    // paint properties with new id by creating another
    // ScopedPaintChunkProperties or by calling Continue() method of the current
    // ScopedPaintChunkProperties. Otherwise the new chunk will use the id of
    // the first display item as its id.
    paint_controller_.UpdateCurrentPaintChunkProperties(nullptr,
                                                        previous_properties_);
  }

  // This can be called after a child ScopedPaintChunkProperties ends, to
  // resume the chunk properties before the child, if the caller can ensure
  // there is no conflict of paint chunk ids.
  void Continue() {
    if (const auto* current_id = paint_controller_.CurrentPaintChunkId()) {
      // There is no child ScopePaintChunkProperties since this scope is
      // created.
      DCHECK(id_ == *current_id);
      return;
    }
    paint_controller_.UpdateCurrentPaintChunkProperties(
        &id_, paint_controller_.CurrentPaintChunkProperties());
  }

 private:
  PaintController& paint_controller_;
  PaintChunk::Id id_;
  PaintChunkProperties previous_properties_;
};

}  // namespace blink

#endif  // ScopedPaintChunkProperties_h
