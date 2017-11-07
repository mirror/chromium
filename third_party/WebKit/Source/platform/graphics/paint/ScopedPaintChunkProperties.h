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
        previous_properties_(paint_controller.CurrentPaintChunkProperties()) {
    PaintChunk::Id id(client, type);
    paint_controller_.UpdateCurrentPaintChunkProperties(&id, properties);
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
  // explicitly set the id of the paint chunk after the child
  // ScopedPaintChunkProperties.
  void Continue(const DisplayItemClient& client, PaintChunk::Type type) {
    PaintChunk::Id id(client, type);
    paint_controller_.UpdateCurrentPaintChunkProperties(
        &id, paint_controller_.CurrentPaintChunkProperties());
  }

 private:
  PaintController& paint_controller_;
  PaintChunkProperties previous_properties_;
};

}  // namespace blink

#endif  // ScopedPaintChunkProperties_h
