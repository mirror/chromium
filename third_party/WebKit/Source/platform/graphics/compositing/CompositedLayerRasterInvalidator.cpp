// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/compositing/CompositedLayerRasterInvalidator.h"

#include "platform/graphics/paint/GeometryMapper.h"

namespace blink {

void CompositedLayerRasterInvalidator::SetTracksRasterInvalidations(
    bool should_track) {
  if (should_track) {
    if (!tracking_info_)
      tracking_info_ = std::make_unique<RasterInvalidationTrackingInfo>();
    tracking_info_->tracking.ClearInvalidations();
    for (const auto& info : paint_chunks_info_) {
      tracking_info_->old_client_debug_names.Set(&info.id.client,
                                                 info.id.client.DebugName());
    }
  } else if (!RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    tracking_info_ = nullptr;
  } else if (tracking_info_) {
    tracking_info_->tracking.ClearInvalidations();
  }
}

IntRect CompositedLayerRasterInvalidator::MapRectFromChunkToLayer(
    const FloatRect& r,
    const PaintChunk& chunk) const {
  FloatClipRect rect(r);
  GeometryMapper::LocalToAncestorVisualRect(
      chunk.properties.property_tree_state, layer_state_, rect);
  if (rect.Rect().IsEmpty())
    return IntRect();

  // Now rect is in the space of the containing transform node of pending_layer,
  // so need to subtract off the layer offset.
  rect.Rect().Move(-layer_bounds_.x(), -layer_bounds_.y());
  rect.Rect().Inflate(chunk.outset_for_raster_effects);
  return ClipByLayerBounds(EnclosingIntRect(rect.Rect()));
}

TransformationMatrix CompositedLayerRasterInvalidator::ChunkToLayerTransform(
    const PaintChunk& chunk) const {
  auto matrix = GeometryMapper::SourceToDestinationProjection(
      chunk.properties.property_tree_state.Transform(),
      layer_state_.Transform());
  matrix.Translate(-layer_bounds_.x(), -layer_bounds_.y());
  return matrix;
}

FloatClipRect CompositedLayerRasterInvalidator::ChunkToLayerClip(
    const PaintChunk& chunk) const {
  auto clip = GeometryMapper::LocalToAncestorClipRect(
      chunk.properties.property_tree_state, layer_state_);
  clip.MoveBy(FloatPoint(-layer_bounds_.x(), -layer_bounds_.y()));
  return clip;
}

size_t CompositedLayerRasterInvalidator::MatchNewChunkToOldChunk(
    const PaintChunk& new_chunk,
    size_t old_index) {
  for (size_t i = old_index; i < paint_chunks_info_.size(); i++) {
    if (paint_chunks_info_[i].Matches(new_chunk))
      return i;
  }
  for (size_t i = 0; i < old_index; i++) {
    if (paint_chunks_info_[i].Matches(new_chunk))
      return i;
  }
  return kNotFound;
}

PaintInvalidationReason
CompositedLayerRasterInvalidator::ChunkPropertiesChanged(
    const PaintChunkInfo& new_chunk,
    const PaintChunkInfo& old_chunk) const {
  if (new_chunk.properties.backface_hidden !=
      old_chunk.properties.backface_hidden)
    return PaintInvalidationReason::kPaintProperty;

  // Special case for transform changes because we may create or delete some
  // transform nodes when no raster invalidation is needed. For example, when
  // a composited layer previously not transformed now gets transformed.
  // Check for real accumulated transform change instead.
  if (new_chunk.chunk_to_layer_transform != old_chunk.chunk_to_layer_transform)
    return PaintInvalidationReason::kPaintProperty;

  // Treat the chunk property as changed if the effect node pointer is
  // different, or the effect node's value changed between the layer state and
  // the chunk state.
  const auto& new_chunk_state = new_chunk.properties.property_tree_state;
  const auto& old_chunk_state = old_chunk.properties.property_tree_state;
  if (new_chunk_state.Effect() != old_chunk_state.Effect() ||
      new_chunk_state.Effect()->Changed(*layer_state_.Effect()))
    return PaintInvalidationReason::kPaintProperty;

  // Check for accumulated clip rect change, if the clip rect is precise.
  if (new_chunk.chunk_to_layer_transform.IsIdentityOr2DTranslation() &&
      !old_chunk.chunk_to_layer_clip.HasRadius() &&
      !new_chunk.chunk_to_layer_clip.HasRadius()) {
    if (new_chunk.chunk_to_layer_clip == old_chunk.chunk_to_layer_clip)
      return PaintInvalidationReason::kNone;
    // Ignore differences out of the current layer bounds.
    if (ClipByLayerBounds(new_chunk.chunk_to_layer_clip.Rect()) ==
        ClipByLayerBounds(old_chunk.chunk_to_layer_clip.Rect()))
      return PaintInvalidationReason::kNone;
    return PaintInvalidationReason::kIncremental;
  }

  // Otherwise treat the chunk property as changed if the clip node pointer is
  // different, or the clip node's value changed between the layer state and the
  // chunk state.
  if (new_chunk_state.Clip() != old_chunk_state.Clip() ||
      new_chunk_state.Clip()->Changed(*layer_state_.Clip()))
    return PaintInvalidationReason::kPaintProperty;

  return PaintInvalidationReason::kNone;
}

// Generates raster invalidations by checking changes (appearing, disappearing,
// reordering, property changes) of chunks. The logic is similar to
// PaintController::GenerateRasterInvalidations(). The complexity is between
// O(n) and O(m*n) where m and n are the numbers of old and new chunks,
// respectively. Normally both m and n are small numbers. The best caseis that
// all old chunks have matching new chunks in the same order. The worst case is
// that no matching chunks except the first one (which always matches otherwise
// we won't reuse the CompositedLayerRasterInvalidator), which is rare. In
// common cases that most of the chunks can be matched in-order, the complexity
// is slightly larger than O(n).
void CompositedLayerRasterInvalidator::GenerateRasterInvalidations(
    const Vector<const PaintChunk*>& new_chunks,
    const Vector<PaintChunkInfo>& new_chunks_info) {
  Vector<bool> old_chunks_matched;
  old_chunks_matched.resize(paint_chunks_info_.size());
  size_t old_index = 0;
  size_t max_matched_old_index = 0;
  for (size_t new_index = 0; new_index < new_chunks.size(); ++new_index) {
    const auto& new_chunk = *new_chunks[new_index];
    const auto& new_chunk_info = new_chunks_info[new_index];

    if (!new_chunk.is_cacheable) {
      FullyInvalidateNewChunk(new_chunk_info,
                              PaintInvalidationReason::kChunkUncacheable);
      continue;
    }

    size_t matched_old_index = MatchNewChunkToOldChunk(new_chunk, old_index);
    if (matched_old_index == kNotFound) {
      // The new chunk doesn't match any old chunk.
      FullyInvalidateNewChunk(new_chunk_info,
                              PaintInvalidationReason::kAppeared);
      continue;
    }

    DCHECK(!old_chunks_matched[matched_old_index]);
    old_chunks_matched[matched_old_index] = true;

    auto& old_chunk_info = paint_chunks_info_[matched_old_index];
    // Clip the old chunk bounds by the new layer bounds.
    old_chunk_info.bounds_in_layer =
        ClipByLayerBounds(old_chunk_info.bounds_in_layer);

    PaintInvalidationReason reason =
        matched_old_index < max_matched_old_index
            ? PaintInvalidationReason::kChunkReordered
            : ChunkPropertiesChanged(new_chunk_info, old_chunk_info);

    // Try to incrementally invalidate, and fall back to full invalidation if
    // incremental invalidation is not suitable.
    if (reason == PaintInvalidationReason::kIncremental &&
        !IncrementallyInvalidateChunk(old_chunk_info, new_chunk_info))
      reason = PaintInvalidationReason::kPaintProperty;

    if (IsFullPaintInvalidationReason(reason)) {
      // Invalidate both old and new bounds of the chunk if the chunk's paint
      // properties changed, or is moved backward and may expose area that was
      // previously covered by it.
      FullyInvalidateChunk(old_chunk_info, new_chunk_info, reason);
      // Ignore the display item raster invalidations because we have fully
      // invalidated the chunk.
    } else {
      // Add the raster invalidations found by PaintController within the chunk.
      AddDisplayItemRasterInvalidations(new_chunk);
    }

    old_index = matched_old_index + 1;
    if (old_index == paint_chunks_info_.size())
      old_index = 0;
    max_matched_old_index = std::max(max_matched_old_index, matched_old_index);
  }

  // Invalidate remaining unmatched (disappeared or uncacheable) old chunks.
  for (size_t i = 0; i < paint_chunks_info_.size(); ++i) {
    if (old_chunks_matched[i])
      continue;
    FullyInvalidateOldChunk(paint_chunks_info_[i],
                            paint_chunks_info_[i].is_cacheable
                                ? PaintInvalidationReason::kDisappeared
                                : PaintInvalidationReason::kChunkUncacheable);
  }
}

void CompositedLayerRasterInvalidator::AddDisplayItemRasterInvalidations(
    const PaintChunk& chunk) {
  DCHECK(chunk.raster_invalidation_tracking.IsEmpty() ||
         chunk.raster_invalidation_rects.size() ==
             chunk.raster_invalidation_tracking.size());

  for (size_t i = 0; i < chunk.raster_invalidation_rects.size(); ++i) {
    auto rect =
        MapRectFromChunkToLayer(chunk.raster_invalidation_rects[i], chunk);
    if (rect.IsEmpty())
      continue;
    raster_invalidation_function_(rect);

    if (!chunk.raster_invalidation_tracking.IsEmpty()) {
      const auto& info = chunk.raster_invalidation_tracking[i];
      tracking_info_->tracking.AddInvalidation(
          info.client, info.client_debug_name, rect, info.reason);
    }
  }
}

bool CompositedLayerRasterInvalidator::IncrementallyInvalidateChunk(
    const PaintChunkInfo& old_chunk,
    const PaintChunkInfo& new_chunk) {
  auto r1 = old_chunk.bounds_in_layer;
  auto r2 = new_chunk.bounds_in_layer;
  bool left_changed = r1.X() != r2.X();
  bool top_changed = r1.Y() != r2.Y();
  bool right_changed = r1.MaxX() != r2.MaxX();
  bool bottom_changed = r1.MaxY() != r2.MaxY();
  int change_count =
      left_changed + top_changed + right_changed + bottom_changed;
  if (change_count == 0)
    return true;
  // Fall back to full invalidation if the bounds changed at more than 1 side.
  if (change_count > 1)
    return false;

  if (r2.Contains(r1))
    std::swap(r1, r2);
  if (left_changed)
    r1.ShiftMaxXEdgeTo(r2.X());
  else if (top_changed)
    r1.ShiftMaxYEdgeTo(r2.Y());
  else if (right_changed)
    r1.ShiftXEdgeTo(r2.MaxX());
  else if (bottom_changed)
    r1.ShiftYEdgeTo(r2.MaxY());

  AddRasterInvalidation(r1, &new_chunk.id.client,
                        PaintInvalidationReason::kIncremental);
  return true;
}

void CompositedLayerRasterInvalidator::FullyInvalidateChunk(
    const PaintChunkInfo& old_chunk,
    const PaintChunkInfo& new_chunk,
    PaintInvalidationReason reason) {
  FullyInvalidateOldChunk(old_chunk, reason);
  if (old_chunk.bounds_in_layer != new_chunk.bounds_in_layer)
    FullyInvalidateNewChunk(new_chunk, reason);
}

void CompositedLayerRasterInvalidator::FullyInvalidateNewChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  AddRasterInvalidation(info.bounds_in_layer, &info.id.client, reason);
}

void CompositedLayerRasterInvalidator::FullyInvalidateOldChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  String debug_name;
  if (tracking_info_)
    debug_name = tracking_info_->old_client_debug_names.at(&info.id.client);
  AddRasterInvalidation(info.bounds_in_layer, &info.id.client, reason,
                        &debug_name);
}

void CompositedLayerRasterInvalidator::AddRasterInvalidation(
    const IntRect& rect,
    const DisplayItemClient* client,
    PaintInvalidationReason reason,
    const String* debug_name) {
  raster_invalidation_function_(rect);
  if (tracking_info_) {
    tracking_info_->tracking.AddInvalidation(
        client, debug_name ? *debug_name : client->DebugName(), rect, reason);
  }
}

RasterInvalidationTracking& CompositedLayerRasterInvalidator::EnsureTracking() {
  if (!tracking_info_)
    tracking_info_ = WTF::MakeUnique<RasterInvalidationTrackingInfo>();
  return tracking_info_->tracking;
}

void CompositedLayerRasterInvalidator::Generate(
    const gfx::Rect& layer_bounds,
    const Vector<const PaintChunk*>& paint_chunks,
    const PropertyTreeState& layer_state,
    const DisplayItemClient* layer_display_item_client) {
  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled())
    EnsureTracking();

  bool layer_bounds_was_empty = layer_bounds_.IsEmpty();
  layer_state_ = layer_state;
  layer_bounds_ = layer_bounds;

  Vector<PaintChunkInfo> new_chunks_info;
  new_chunks_info.ReserveCapacity(paint_chunks.size());
  for (const auto* chunk : paint_chunks) {
    new_chunks_info.push_back(PaintChunkInfo(
        MapRectFromChunkToLayer(chunk->bounds, *chunk),
        ChunkToLayerTransform(*chunk), ChunkToLayerClip(*chunk), *chunk));
    if (tracking_info_) {
      tracking_info_->new_client_debug_names.insert(
          &chunk->id.client, chunk->id.client.DebugName());
    }
  }

  if (!layer_bounds_was_empty && !layer_bounds_.IsEmpty())
    GenerateRasterInvalidations(paint_chunks, new_chunks_info);

  paint_chunks_info_.clear();
  std::swap(paint_chunks_info_, new_chunks_info);
  if (tracking_info_) {
    tracking_info_->old_client_debug_names.clear();
    std::swap(tracking_info_->old_client_debug_names,
              tracking_info_->new_client_debug_names);
  }
}

}  // namespace blink
