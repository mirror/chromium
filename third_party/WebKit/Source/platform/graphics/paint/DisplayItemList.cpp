// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/paint/DisplayItemList.h"

#include "platform/graphics/LoggingCanvas.h"
#include "platform/graphics/paint/DrawingDisplayItem.h"
#include "platform/graphics/paint/PaintChunk.h"

namespace blink {

DisplayItemList::Range<DisplayItemList::iterator>
DisplayItemList::ItemsInPaintChunk(const PaintChunk& paint_chunk) {
  return Range<iterator>(begin() + paint_chunk.begin_index,
                         begin() + paint_chunk.end_index);
}

DisplayItemList::Range<DisplayItemList::const_iterator>
DisplayItemList::ItemsInPaintChunk(const PaintChunk& paint_chunk) const {
  return Range<const_iterator>(begin() + paint_chunk.begin_index,
                               begin() + paint_chunk.end_index);
}

#if DCHECK_IS_ON()

std::unique_ptr<JSONArray> DisplayItemList::SubsequenceAsJSON(
    size_t begin_index,
    size_t end_index,
    JsonFlags flags) const {
  auto json_array = JSONArray::Create();
  AppendSubsequenceAsJSON(begin_index, end_index, flags, *json_array);
  return json_array;
}

void DisplayItemList::AppendSubsequenceAsJSON(size_t begin_index,
                                              size_t end_index,
                                              JsonFlags flags,
                                              JSONArray& json_array) const {
  for (size_t i = begin_index; i < end_index; ++i) {
    std::unique_ptr<JSONObject> json = JSONObject::Create();

    const auto& item = (*this)[i];
    if ((flags & kSkipNonDrawings) && !item.IsDrawing())
      continue;

    json->SetInteger("index", i);

    if (flags & kShownOnlyDisplayItemTypes) {
      json->SetString("type", DisplayItem::TypeAsDebugString(item.GetType()));
    } else {
      json->SetString("clientDebugName", ClientDebugName(item.Client(), flags));
      item.PropertiesAsJSON(*json);
    }

#ifndef NDEBUG
    if ((flags & kShowPaintRecords) && item.IsDrawing()) {
      const auto& drawing_item = static_cast<const DrawingDisplayItem&>(item);
      if (const auto* record = drawing_item.GetPaintRecord().get())
        json->SetArray("record", RecordAsJSON(*record));
    }
#endif

    json_array.PushObject(std::move(json));
  }
}

String DisplayItemList::ClientDebugName(const DisplayItemClient& client,
                                        JsonFlags flags) {
  if (flags & kShowClientDebugName) {
    DCHECK(client.IsAlive());
    return client.DebugName();
  }

  // We are showing a client in the old display list. Show client name if we
  // are sure the client is still alive.
  if (client.IsAlive() &&
      // The client is not a new client created at the same address after the
      // old client was destroyed.
      !client.IsJustCreated())
    return client.DebugName();

  return "DEAD";
}

#endif  // DCHECK_IS_ON()

}  // namespace blink
