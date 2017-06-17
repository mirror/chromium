// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/compositing/ContentLayerClientImpl.h"

#include "platform/graphics/compositing/PaintChunksToCcLayer.h"
#include "platform/graphics/paint/GeometryMapper.h"
#include "platform/graphics/paint/PaintArtifact.h"
#include "platform/graphics/paint/PaintChunk.h"
#include "platform/graphics/paint/RasterInvalidationTracking.h"
#include "platform/json/JSONValues.h"

#include <bitset>

namespace blink {

template <typename T>
static std::unique_ptr<JSONArray> PointAsJSONArray(const T& point) {
  std::unique_ptr<JSONArray> array = JSONArray::Create();
  array->PushDouble(point.X());
  array->PushDouble(point.Y());
  return array;
}

template <typename T>
static std::unique_ptr<JSONArray> SizeAsJSONArray(const T& size) {
  std::unique_ptr<JSONArray> array = JSONArray::Create();
  array->PushDouble(size.Width());
  array->PushDouble(size.Height());
  return array;
}

void ContentLayerClientImpl::SetTracksRasterInvalidations(bool value) {
  if (value) {
    raster_invalidation_tracking_info_ =
        WTF::MakeUnique<RasterInvalidationTrackingInfo>();

    for (const auto& info : paint_chunks_info_) {
      if (info.id) {
        raster_invalidation_tracking_info_->old_client_debug_names.Set(
            &info.id->client, info.id->client.DebugName());
      }
    }
  } else if (!RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    raster_invalidation_tracking_info_ = nullptr;
  } else if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->tracking.invalidations.clear();
  }
}

std::unique_ptr<JSONObject> ContentLayerClientImpl::LayerAsJSON(
    LayerTreeFlags flags) {
  std::unique_ptr<JSONObject> json = JSONObject::Create();
  json->SetString("name", debug_name_);

  FloatPoint position(cc_picture_layer_->offset_to_transform_parent().x(),
                      cc_picture_layer_->offset_to_transform_parent().y());
  if (position != FloatPoint())
    json->SetArray("position", PointAsJSONArray(position));

  IntSize bounds(cc_picture_layer_->bounds().width(),
                 cc_picture_layer_->bounds().height());
  if (!bounds.IsEmpty())
    json->SetArray("bounds", SizeAsJSONArray(bounds));

  json->SetBoolean("contentsOpaque", cc_picture_layer_->contents_opaque());
  json->SetBoolean("drawsContent", cc_picture_layer_->DrawsContent());

  if (flags & kLayerTreeIncludesDebugInfo) {
    std::unique_ptr<JSONArray> paint_chunk_contents_array = JSONArray::Create();
    for (const auto& debug_data : paint_chunk_debug_data_) {
      paint_chunk_contents_array->PushValue(debug_data->Clone());
    }
    json->SetArray("paintChunkContents", std::move(paint_chunk_contents_array));
  }

  if (raster_invalidation_tracking_info_)
    raster_invalidation_tracking_info_->tracking.AsJSON(json.get());

  return json;
}

IntRect ContentLayerClientImpl::MapRasterInvalidationRectFromChunkToLayer(
    const FloatRect& r,
    const PaintChunk& chunk,
    const PaintChunk& first_chunk) const {
  FloatClipRect rect(r);
  GeometryMapper::LocalToAncestorVisualRect(
      chunk.properties.property_tree_state,
      first_chunk.properties.property_tree_state, rect);
  if (rect.Rect().IsEmpty())
    return IntRect();

  // Now rect is in the space of the containing transform node of pending_layer,
  // so need to subtract off the layer offset.
  rect.Rect().Move(-layer_bounds_.x(), -layer_bounds_.y());
  rect.Rect().Inflate(chunk.outset_for_raster_effects);
  return EnclosingIntRect(rect.Rect());
}

// Generates raster invalidations by checking changes (appearing, disappearing,
// reordering, property changes) of chunks. The logic is similar to
// PaintController::GenerateRasterInvalidations(). The complexity is between
// O(n) and O((m-1)*(n-1)+1) where m is the number of old chunks and n is the
// number of new chunks. Normally both m and n are small numbers. The best case
// is that all old chunks have matching new chunks in the same order. The worst
// case is that no matching chunks except the first one (which always matches
// otherwise we won't reuse the ContentLayerClientImpl).
void ContentLayerClientImpl::GenerateRasterInvalidations(
    const Vector<const PaintChunk*>& new_chunks) {
  size_t first_unmatched_old_index = 0;
  size_t old_index = 0;
  size_t new_index = 0;

  Vector<bool> old_chunks_matched;
  old_chunks_matched.resize(paint_chunks_info_.size());
  Vector<PaintChunkInfo> new_chunks_info;
  new_chunks_info.resize(new_chunks.size());
  const auto& first_chunk = *new_chunks[0];

  while (new_index < new_chunks.size() &&
         first_unmatched_old_index < paint_chunks_info_.size()) {
    const auto& old_chunk_info = paint_chunks_info_[old_index];
    const auto& new_chunk = *new_chunks[new_index];
    auto& new_chunk_info = new_chunks_info[new_index];
    UpdatePaintChunkInfo(new_chunk, first_chunk, new_chunk_info);
    ++new_index;

    if (!new_chunk.id) {
      // This chunk is not cacheable, so always invalidate the whole chunk.
      InvalidateRasterForChunk(new_chunk_info, PaintInvalidationReason::kFull);
      continue;
    }

    bool matched = new_chunk.Matches(old_chunk_info.id);
    if (!matched) {
      // Search |paint_chunks_info_| for old chunk matching |new_chunk|.
      size_t i = old_index;
      do {
        ++i;
        if (i == paint_chunks_info_.size())
          i = first_unmatched_old_index;
        if (!old_chunks_matched[i] &&
            new_chunk.Matches(paint_chunks_info_[i].id)) {
          matched = true;
          old_index = i;
        }
      } while (i != old_index);
    }

    if (!matched) {
      InvalidateRasterForChunk(new_chunk_info,
                               PaintInvalidationReason::kAppeared);
      continue;
    }

    DCHECK(!old_chunks_matched[old_index]);
    old_chunks_matched[old_index] = true;
    bool in_order = old_index == first_unmatched_old_index;
    if (in_order) {
      // TODO(wangxianzhu): Fully invalidate for paint property changes.
      // Add the raster invalidations found by PaintController within the chunk.
      AddDisplayItemRasterInvalidations(new_chunk, first_chunk);
    } else {
      // Invalidate both old and new bounds of the chunk if the chunk is moved
      // forward and may expose area that was previously covered by other
      // chunks.
      auto reason = PaintInvalidationReason::kStyle;
      InvalidateRasterForChunk(old_chunk_info, reason);
      if (old_chunk_info.bounds_in_layer != new_chunk_info.bounds_in_layer)
        InvalidateRasterForChunk(new_chunk_info, reason);
      // Ignore the display item raster invalidations because we have fully
      // invalidated the chunk.
    }

    // Adjust |old_index| and |first_unmatched_old_index| for the next
    // iteration.
    do {
      ++old_index;
    } while (old_index < paint_chunks_info_.size() &&
             old_chunks_matched[old_index]);
    if (in_order)
      first_unmatched_old_index = old_index;
    else if (old_index == paint_chunks_info_.size())
      old_index = first_unmatched_old_index;
  }

  // Invalidate remaining unmatched new chunks.
  for (size_t i = new_index; i < new_chunks.size(); ++i) {
    UpdatePaintChunkInfo(*new_chunks[i], first_chunk, new_chunks_info[i]);
    InvalidateRasterForChunk(new_chunks_info[i],
                             PaintInvalidationReason::kAppeared);
  }

  // Invalidate remaining unmatched old chunks.
  for (size_t i = first_unmatched_old_index; i < paint_chunks_info_.size();
       ++i) {
    if (!old_chunks_matched[i]) {
      InvalidateRasterForChunk(paint_chunks_info_[i],
                               PaintInvalidationReason::kDisappeared);
    }
  }

  paint_chunks_info_.clear();
  std::swap(paint_chunks_info_, new_chunks_info);

  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->old_client_debug_names.clear();
    std::swap(raster_invalidation_tracking_info_->old_client_debug_names,
              raster_invalidation_tracking_info_->new_client_debug_names);
  }
}

void ContentLayerClientImpl::AddDisplayItemRasterInvalidations(
    const PaintChunk& chunk,
    const PaintChunk& first_chunk) {
  DCHECK(chunk.raster_invalidation_tracking.IsEmpty() ||
         chunk.raster_invalidation_rects.size() ==
             chunk.raster_invalidation_tracking.size());

  for (size_t i = 0; i < chunk.raster_invalidation_rects.size(); ++i) {
    auto rect = MapRasterInvalidationRectFromChunkToLayer(
        chunk.raster_invalidation_rects[i], chunk, first_chunk);
    if (rect.IsEmpty())
      continue;
    cc_picture_layer_->SetNeedsDisplayRect(rect);

    if (!raster_invalidation_tracking_info_ ||
        chunk.raster_invalidation_tracking.IsEmpty())
      continue;
    auto info = chunk.raster_invalidation_tracking[i];
    info.rect = rect;
    raster_invalidation_tracking_info_->tracking.AddInvalidation(info);
  }
}

void ContentLayerClientImpl::InvalidateRasterForChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  cc_picture_layer_->SetNeedsDisplayRect(info.bounds_in_layer);

  if (!raster_invalidation_tracking_info_)
    return;
  RasterInvalidationInfo raster_info;
  raster_info.rect = info.bounds_in_layer;
  raster_info.reason = reason;
  if (!info.id) {
    raster_info.client_debug_name = "Unknown";
  } else if (reason == PaintInvalidationReason::kDisappeared) {
    raster_info.client_debug_name =
        raster_invalidation_tracking_info_->old_client_debug_names.at(
            &info.id->client);
  } else {
    raster_info.client_debug_name = info.id->client.DebugName();
  }
  raster_invalidation_tracking_info_->tracking.AddInvalidation(raster_info);
}

void ContentLayerClientImpl::InvalidateRasterForWholeLayer() {
  IntRect rect(0, 0, layer_bounds_.width(), layer_bounds_.height());
  cc_picture_layer_->SetNeedsDisplayRect(rect);

  if (!raster_invalidation_tracking_info_)
    return;
  RasterInvalidationInfo info;
  info.rect = rect;
  info.reason = PaintInvalidationReason::kGeometry;
  info.client_debug_name = debug_name_;
  raster_invalidation_tracking_info_->tracking.AddInvalidation(info);
}

void ContentLayerClientImpl::UpdatePaintChunkInfo(const PaintChunk& chunk,
                                                  const PaintChunk& first_chunk,
                                                  PaintChunkInfo& info) const {
  info.bounds_in_layer = MapRasterInvalidationRectFromChunkToLayer(
      chunk.bounds, chunk, first_chunk);
  if (chunk.id) {
    info.id.emplace(chunk.id->client, chunk.id->type);
    if (raster_invalidation_tracking_info_) {
      raster_invalidation_tracking_info_->new_client_debug_names.insert(
          &chunk.id->client, chunk.id->client.DebugName());
    }
  }
}

scoped_refptr<cc::PictureLayer> ContentLayerClientImpl::UpdateCcPictureLayer(
    const PaintArtifact& paint_artifact,
    const FloatRect& bounds,
    const Vector<const PaintChunk*>& paint_chunks,
    gfx::Vector2dF& layer_offset,
    bool store_debug_info) {
  const auto& first_chunk = *paint_chunks[0];
  debug_name_ =
      (first_chunk.id ? first_chunk.id->client
                      : paint_artifact.GetDisplayItemList()[0].Client())
          .DebugName();

  paint_chunk_debug_data_.clear();

  if (store_debug_info) {
    for (const auto& paint_chunk : paint_chunks) {
      paint_chunk_debug_data_.push_back(
          paint_artifact.GetDisplayItemList().SubsequenceAsJSON(
              paint_chunk->begin_index, paint_chunk->end_index,
              DisplayItemList::kSkipNonDrawings |
                  DisplayItemList::kShownOnlyDisplayItemTypes));
    }
  }

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      !raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_ =
        WTF::MakeUnique<RasterInvalidationTrackingInfo>();
  }

  gfx::Rect new_layer_bounds = EnclosingIntRect(bounds);
  bool layer_origin_changed =
      layer_bounds_.origin() != new_layer_bounds.origin();
  layer_bounds_ = new_layer_bounds;
  cc_picture_layer_->SetBounds(new_layer_bounds.size());
  layer_offset = new_layer_bounds.OffsetFromOrigin();

  if (layer_origin_changed)
    InvalidateRasterForWholeLayer();
  else
    GenerateRasterInvalidations(paint_chunks);

  Optional<RasterUnderInvalidationCheckingParams> params;
  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    params.emplace(raster_invalidation_tracking_info_->tracking,
                   IntRect(0, 0, layer_bounds_.width(), layer_bounds_.height()),
                   debug_name_);
  }
  cc_display_item_list_ = PaintChunksToCcLayer::Convert(
      paint_chunks, first_chunk.properties.property_tree_state, layer_offset,
      paint_artifact.GetDisplayItemList(), params ? &*params : nullptr);

  return cc_picture_layer_;
}

}  // namespace blink
