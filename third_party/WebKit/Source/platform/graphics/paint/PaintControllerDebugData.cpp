// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/paint/PaintController.h"

#include "platform/graphics/paint/DrawingDisplayItem.h"

#if DCHECK_IS_ON()

namespace blink {

class PaintController::DisplayItemListAsJSON {
  STACK_ALLOCATED();

 public:
  DisplayItemListAsJSON(const DisplayItemList&,
                        const CachedSubsequenceMap&,
                        const Vector<PaintChunk>&,
                        bool show_paint_records,
                        bool for_new_display_item_list);

  String ToString() {
    return SubsequenceAsJSONArrayRecursive(0, list_.size())
        ->ToPrettyJSONString();
  }

 private:
  std::unique_ptr<JSONObject> SubsequenceAsJSONObjectRecursive();
  std::unique_ptr<JSONArray> SubsequenceAsJSONArrayRecursive(size_t, size_t);
  void AppendSubsequenceAsJSON(size_t, size_t, JSONArray&);
  String ClientName(const DisplayItemClient&) const;

  struct SubsequenceInfo {
    SubsequenceInfo(const DisplayItemClient* client, size_t start, size_t end)
        : client(client), start(start), end(end) {}
    const DisplayItemClient* client;
    size_t start;
    size_t end;
  };

  const DisplayItemList& list_;
  Vector<SubsequenceInfo> subsequences_;
  Vector<SubsequenceInfo>::const_iterator current_subsequence_;
  const Vector<PaintChunk>& chunks_;
  Vector<PaintChunk>::const_iterator current_chunk_;
  bool show_paint_records_;
  bool for_new_display_item_list_;
};

PaintController::DisplayItemListAsJSON::DisplayItemListAsJSON(
    const DisplayItemList& list,
    const CachedSubsequenceMap& subsequence_map,
    const Vector<PaintChunk>& chunks,
    bool show_paint_records,
    bool for_new_display_item_list)
    : list_(list),
      chunks_(chunks),
      current_chunk_(chunks.begin()),
      show_paint_records_(show_paint_records),
      for_new_display_item_list_(for_new_display_item_list) {
  for (const auto& item : subsequence_map) {
    subsequences_.push_back(
        SubsequenceInfo(item.key, item.value.start, item.value.end));
  }
  std::sort(subsequences_.begin(), subsequences_.end(),
            [](const SubsequenceInfo& a, const SubsequenceInfo& b) {
              return a.start == b.start ? a.end > b.end : a.start < b.start;
            });

  current_subsequence_ = subsequences_.begin();
}

std::unique_ptr<JSONObject>
PaintController::DisplayItemListAsJSON::SubsequenceAsJSONObjectRecursive() {
  const auto& subsequence = *current_subsequence_;
  ++current_subsequence_;

  auto json_object = JSONObject::Create();

  json_object->SetString("subsequence", ClientName(*subsequence.client));
  json_object->SetArray(
      RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ? "chunks"
                                                         : "displayItems",
      SubsequenceAsJSONArrayRecursive(subsequence.start, subsequence.end));

  return json_object;
}

std::unique_ptr<JSONArray>
PaintController::DisplayItemListAsJSON::SubsequenceAsJSONArrayRecursive(
    size_t start_item,
    size_t end_item) {
  std::unique_ptr<JSONArray> array = JSONArray::Create();
  size_t item_index = start_item;

  while (current_subsequence_ != subsequences_.end() &&
         current_subsequence_->start < end_item) {
    const auto& subsequence = *current_subsequence_;
    DCHECK(subsequence.start >= item_index);
    DCHECK(subsequence.end <= end_item);

    if (item_index < subsequence.start)
      AppendSubsequenceAsJSON(item_index, subsequence.start, *array);
    array->PushObject(SubsequenceAsJSONObjectRecursive());
    item_index = subsequence.end;
  }

  if (item_index < end_item)
    AppendSubsequenceAsJSON(item_index, end_item, *array);

  return array;
}

void PaintController::DisplayItemListAsJSON::AppendSubsequenceAsJSON(
    size_t start_item,
    size_t end_item,
    JSONArray& json_array) {
  DCHECK(end_item > start_item);

  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    list_.AppendSubsequenceAsJSON(start_item, end_item, show_paint_records_,
                                  json_array);
    return;
  }

  DCHECK(current_chunk_ != chunks_.end());
  DCHECK(current_chunk_->begin_index == start_item);

  while (current_chunk_ != chunks_.end() &&
         current_chunk_->end_index <= end_item) {
    const auto& chunk = *current_chunk_;
    auto json_object = JSONObject::Create();

    String chunk_name = ClientName(chunk.id.client);
    if (chunk.id.type != DisplayItem::kUninitializedType) {
      chunk_name.append(" type: ");
      chunk_name.append(DisplayItem::TypeAsDebugString(chunk.id.type));
    }
    json_object->SetString("chunk", chunk_name);

    json_object->SetArray(
        "displayItems",
        list_.SubsequenceAsJSON(chunk.begin_index, chunk.end_index,
                                show_paint_records_
                                    ? DisplayItemList::kShowPaintRecords
                                    : DisplayItemList::kDefault));

    json_array.PushObject(std::move(json_object));
    ++current_chunk_;
  }
}

String PaintController::DisplayItemListAsJSON::ClientName(
    const DisplayItemClient& client) const {
  String result = String::Format("client: %p", &client);
  result.append(' ');
  if (for_new_display_item_list_) {
    DCHECK(client.IsAlive());
    result.append(client.DebugName());
  } else if (client.IsAlive() && !client.IsJustCreated()) {
    result.append(client.DebugName());
  } else {
    result.append("dead");
  }
  return result;
}

void PaintController::ShowDebugDataInternal(bool show_paint_records) const {
  DLOG(INFO) << "current display item list: "
             << DisplayItemListAsJSON(
                    current_paint_artifact_.GetDisplayItemList(),
                    current_cached_subsequences_,
                    current_paint_artifact_.PaintChunks(), show_paint_records,
                    false)
                    .ToString()
                    .Utf8()
                    .data();

  DLOG(INFO) << "new display item list: "
             << DisplayItemListAsJSON(
                    new_display_item_list_, new_cached_subsequences_,
                    new_paint_chunks_.PaintChunks(), show_paint_records, true)
                    .ToString()
                    .Utf8()
                    .data();
}

void PaintController::ShowDebugData() const {
  return ShowDebugDataInternal(false);
}

#ifndef NDEBUG
void PaintController::ShowDebugDataWithRecords() const {
  return ShowDebugDataInternal(true);
}
#endif

}  // namespace blink

#endif  // DCHECK_IS_ON()
