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

void ContentLayerClientImpl::SetTracksRasterInvalidations(bool should_track) {
  if (should_track) {
    raster_invalidation_tracking_info_ =
        WTF::MakeUnique<RasterInvalidationTrackingInfo>();

    for (const auto& info : paint_chunks_info_) {
      raster_invalidation_tracking_info_->old_client_debug_names.Set(
          &info.id.client, info.id.client.DebugName());
    }
  } else if (!RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    raster_invalidation_tracking_info_ = nullptr;
  } else if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->tracking.invalidations.clear();
  }
}

const Vector<RasterInvalidationInfo>&
ContentLayerClientImpl::TrackedRasterInvalidations() const {
  return raster_invalidation_tracking_info_->tracking.invalidations;
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
    const PropertyTreeState& layer_state) const {
  FloatClipRect rect(r);
  GeometryMapper::LocalToAncestorVisualRect(
      chunk.properties.property_tree_state, layer_state, rect);
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
// O(n) and O(m*n) where m and n are the numbers of old and new chunks,
// respectively. Normally both m and n are small numbers. The best caseis that
// all old chunks have matching new chunks in the same order. The worst case is
// that no matching chunks except the first one (which always matches otherwise
// we won't reuse the ContentLayerClientImpl), which is rare. In common cases
// that most of the chunks can be matched in-order, the complexity is slightly
// larger than O(n).
void ContentLayerClientImpl::GenerateRasterInvalidations(
    const Vector<const PaintChunk*>& new_chunks,
    const Vector<PaintChunkInfo>& new_chunks_info,
    const PropertyTreeState& layer_state) {
  Vector<bool> old_chunks_matched_or_uncacheable;
  old_chunks_matched_or_uncacheable.resize(paint_chunks_info_.size());
  size_t old_index = paint_chunks_info_.size();
  for (size_t i = 0; i < paint_chunks_info_.size(); ++i) {
    if (paint_chunks_info_[i].is_cacheable) {
      old_index = std::min(i, old_index);
    } else {
      InvalidateRasterForOldChunk(paint_chunks_info_[i],
                                  PaintInvalidationReason::kChunkUncacheable);
      old_chunks_matched_or_uncacheable[i] = true;
    }
  }
  size_t first_unmatched_old_index = old_index;

  for (size_t new_index = 0; new_index < new_chunks.size(); ++new_index) {
    const auto* old_chunk_info = old_index < paint_chunks_info_.size()
                                     ? &paint_chunks_info_[old_index]
                                     : nullptr;
    DCHECK(!old_chunk_info || old_chunk_info->is_cacheable);
    const auto& new_chunk = *new_chunks[new_index];
    const auto& new_chunk_info = new_chunks_info[new_index];

    if (!new_chunk.is_cacheable) {
      InvalidateRasterForNewChunk(new_chunk_info,
                                  PaintInvalidationReason::kChunkUncacheable);
      continue;
    }

    bool matched = old_chunk_info && new_chunk.Matches(old_chunk_info->id);
    if (old_chunk_info && !matched) {
      // Search |paint_chunks_info_| for old chunk matching |new_chunk|.
      size_t i = old_index;
      do {
        ++i;
        if (i == paint_chunks_info_.size())
          i = first_unmatched_old_index;
        if (!old_chunks_matched_or_uncacheable[i] &&
            new_chunk.Matches(paint_chunks_info_[i].id)) {
          matched = true;
          old_index = i;
          old_chunk_info = &paint_chunks_info_[i];
        }
      } while (i != old_index);
    }

    if (!matched) {
      InvalidateRasterForNewChunk(new_chunk_info,
                                  PaintInvalidationReason::kAppeared);
      continue;
    }

    DCHECK(!old_chunks_matched_or_uncacheable[old_index]);
    old_chunks_matched_or_uncacheable[old_index] = true;
    bool in_order = old_index == first_unmatched_old_index;
    if (in_order) {
      // TODO(wangxianzhu): Also fully invalidate for paint property changes.
      // Add the raster invalidations found by PaintController within the chunk.
      AddDisplayItemRasterInvalidations(new_chunk, layer_state);
    } else {
      DCHECK(old_index > first_unmatched_old_index);
      // Invalidate both old and new bounds of the chunk if the chunk is moved
      // backward and may expose area that was previously covered by it.
      InvalidateRasterForOldChunk(*old_chunk_info,
                                  PaintInvalidationReason::kChunkReordered);
      if (old_chunk_info->bounds_in_layer != new_chunk_info.bounds_in_layer) {
        InvalidateRasterForNewChunk(new_chunk_info,
                                    PaintInvalidationReason::kChunkReordered);
      }
      // Ignore the display item raster invalidations because we have fully
      // invalidated the chunk.
    }

    // Adjust |old_index| and |first_unmatched_old_index| for the next
    // iteration.
    do {
      ++old_index;
    } while (old_index < paint_chunks_info_.size() &&
             old_chunks_matched_or_uncacheable[old_index]);
    if (in_order)
      first_unmatched_old_index = old_index;
    else if (old_index == paint_chunks_info_.size())
      old_index = first_unmatched_old_index;
  }

  // Invalidate remaining unmatched old chunks.
  for (size_t i = first_unmatched_old_index; i < paint_chunks_info_.size();
       ++i) {
    if (!old_chunks_matched_or_uncacheable[i]) {
      InvalidateRasterForOldChunk(paint_chunks_info_[i],
                                  PaintInvalidationReason::kDisappeared);
    }
  }
}

void ContentLayerClientImpl::AddDisplayItemRasterInvalidations(
    const PaintChunk& chunk,
    const PropertyTreeState& layer_state) {
  DCHECK(chunk.raster_invalidation_tracking.IsEmpty() ||
         chunk.raster_invalidation_rects.size() ==
             chunk.raster_invalidation_tracking.size());

  for (size_t i = 0; i < chunk.raster_invalidation_rects.size(); ++i) {
    auto rect = MapRasterInvalidationRectFromChunkToLayer(
        chunk.raster_invalidation_rects[i], chunk, layer_state);
    if (rect.IsEmpty())
      continue;
    cc_picture_layer_->SetNeedsDisplayRect(rect);

    if (!chunk.raster_invalidation_tracking.IsEmpty()) {
      const auto& info = chunk.raster_invalidation_tracking[i];
      raster_invalidation_tracking_info_->tracking.AddInvalidation(
          info.client, info.client_debug_name, rect, info.reason);
    }
  }
}

void ContentLayerClientImpl::InvalidateRasterForNewChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  cc_picture_layer_->SetNeedsDisplayRect(info.bounds_in_layer);

  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->tracking.AddInvalidation(
        &info.id.client, info.id.client.DebugName(), info.bounds_in_layer,
        reason);
  }
}

void ContentLayerClientImpl::InvalidateRasterForOldChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  cc_picture_layer_->SetNeedsDisplayRect(info.bounds_in_layer);

  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->tracking.AddInvalidation(
        &info.id.client,
        raster_invalidation_tracking_info_->old_client_debug_names.at(
            &info.id.client),
        info.bounds_in_layer, reason);
  }
}

void ContentLayerClientImpl::InvalidateRasterForWholeLayer() {
  IntRect rect(0, 0, layer_bounds_.width(), layer_bounds_.height());
  cc_picture_layer_->SetNeedsDisplayRect(rect);

  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->tracking.AddInvalidation(
        &paint_chunks_info_[0].id.client, debug_name_, rect,
        PaintInvalidationReason::kFullLayer);
  }
}

scoped_refptr<cc::PictureLayer> ContentLayerClientImpl::UpdateCcPictureLayer(
    const PaintArtifact& paint_artifact,
    const gfx::Rect& layer_bounds,
    const Vector<const PaintChunk*>& paint_chunks,
    const PropertyTreeState& layer_state,
    bool store_debug_info) {
  // TODO(wangxianzhu): Avoid calling DebugName() in official release build.
  debug_name_ = paint_chunks[0]->id.client.DebugName();

  paint_chunk_debug_data_.clear();
  if (store_debug_info) {
    for (const auto* chunk : paint_chunks) {
      paint_chunk_debug_data_.push_back(
          paint_artifact.GetDisplayItemList().SubsequenceAsJSON(
              chunk->begin_index, chunk->end_index,
              DisplayItemList::kSkipNonDrawings |
                  DisplayItemList::kShownOnlyDisplayItemTypes));
    }
  }

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      !raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_ =
        WTF::MakeUnique<RasterInvalidationTrackingInfo>();
  }

  bool layer_bounds_was_empty = layer_bounds_.IsEmpty();
  bool layer_origin_changed = layer_bounds_.origin() != layer_bounds.origin();
  layer_bounds_ = layer_bounds;
  cc_picture_layer_->SetBounds(layer_bounds.size());
  cc_picture_layer_->SetIsDrawable(true);

  Vector<PaintChunkInfo> new_chunks_info;
  new_chunks_info.ReserveCapacity(paint_chunks.size());
  for (const auto* chunk : paint_chunks) {
    new_chunks_info.push_back(
        PaintChunkInfo(MapRasterInvalidationRectFromChunkToLayer(
                           chunk->bounds, *chunk, layer_state),
                       *chunk));
    if (raster_invalidation_tracking_info_) {
      raster_invalidation_tracking_info_->new_client_debug_names.insert(
          &chunk->id.client, chunk->id.client.DebugName());
    }
  }

  if (!layer_bounds_was_empty && !layer_bounds_.IsEmpty()) {
    if (layer_origin_changed)
      InvalidateRasterForWholeLayer();
    else
      GenerateRasterInvalidations(paint_chunks, new_chunks_info, layer_state);
  }

  Optional<RasterUnderInvalidationCheckingParams> params;
  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    params.emplace(raster_invalidation_tracking_info_->tracking,
                   IntRect(0, 0, layer_bounds_.width(), layer_bounds_.height()),
                   debug_name_);
  }
  cc_display_item_list_ = PaintChunksToCcLayer::Convert(
      paint_chunks, layer_state, layer_bounds.OffsetFromOrigin(),
      paint_artifact.GetDisplayItemList(), params ? &*params : nullptr);

  paint_chunks_info_.clear();
  std::swap(paint_chunks_info_, new_chunks_info);
  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->old_client_debug_names.clear();
    std::swap(raster_invalidation_tracking_info_->old_client_debug_names,
              raster_invalidation_tracking_info_->new_client_debug_names);
  }

  return cc_picture_layer_;
}

}  // namespace blink
