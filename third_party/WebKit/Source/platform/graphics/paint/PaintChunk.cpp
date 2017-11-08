// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/paint/PaintChunk.h"

#include "platform/wtf/text/WTFString.h"

namespace blink {

String PaintChunk::TypeAsDebugString(Type type) {
  if (type < kLayerChunk)
    return DisplayItem::TypeAsDebugString(static_cast<DisplayItem::Type>(type));

  switch (type) {
    case kLayerChunk:
      return "LayerChunk";
    case kLayerChunkAfterNegativeZOrderChildren:
      return "LayerChunkAfterNegativeZOrderChildren";
    case kLayerChunkMask:
      return "LayerChunkMask";
  }
  NOTREACHED();
  return "Unknown";
}

String PaintChunk::ToString() const {
  return String::Format(
      "begin=%zu, end=%zu, id=%p:"
#if DCHECK_IS_ON()
      "%s "
#else
      "%d "
#endif
      "cacheable=%d props=(%s) bounds=%s known_to_be_opaque=%d "
      "raster_invalidation_rects=%zu",
      begin_index, end_index, &id.client,
#if DCHECK_IS_ON()
      TypeAsDebugString(id.type).Ascii().data(),
#else
      static_cast<int>(id.type),
#endif
      is_cacheable, properties.ToString().Ascii().data(),
      bounds.ToString().Ascii().data(), known_to_be_opaque,
      raster_invalidation_rects.size());
}

}  // namespace blink
