// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositedLayerRasterInvalidator_h
#define CompositedLayerRasterInvalidator_h

#include "platform/graphics/paint/FloatClipRect.h"
#include "platform/graphics/paint/PaintChunk.h"
#include "platform/graphics/paint/RasterInvalidationTracking.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/Vector.h"

namespace blink {

class IntRect;

class PLATFORM_EXPORT CompositedLayerRasterInvalidator {
 public:
  using RasterInvalidationFunction = std::function<void(const IntRect&)>;

  CompositedLayerRasterInvalidator(
      RasterInvalidationFunction raster_invalidation_function)
      : raster_invalidation_function_(raster_invalidation_function),
        layer_state_(nullptr, nullptr, nullptr) {}

  void SetTracksRasterInvalidations(bool);
  RasterInvalidationTracking* GetTracking() const {
    return tracking_info_ ? &tracking_info_->tracking : nullptr;
  }

  RasterInvalidationTracking& EnsureTracking();

  void Generate(const gfx::Rect& layer_bounds,
                const Vector<const PaintChunk*>&,
                const PropertyTreeState&,
                // For SPv175 only. For SPv2 we can get it from the first chunk
                // which always exists.
                const DisplayItemClient* layer_display_item_client = nullptr);

  // This is for SPv175 when a new painting has nothing changed. A chunk may
  // have property tree state escaping the layer and it may need raster
  // invalidation even if there is no change in painting of this layer.
  // A test case is
  // paint/invalidation/compositing/fixed-pos-inside-composited-intermediate-layer.html.
  void GenerateForPropertyChanges(const Vector<const PaintChunk*>&);

  bool Matches(const PaintChunk& paint_chunk) const {
    return paint_chunks_info_.size() && paint_chunks_info_[0].is_cacheable &&
           paint_chunk.Matches(paint_chunks_info_[0].id);
  }

  const PropertyTreeState& GetLayerState() const { return layer_state_; }
  const gfx::Rect& LayerBounds() const { return layer_bounds_; }

 private:
  friend class CompositedLayerRasterInvalidatorTest;

  struct PaintChunkInfo {
    PaintChunkInfo(const IntRect& bounds,
                   const TransformationMatrix& chunk_to_layer_transform,
                   const FloatClipRect& chunk_to_layer_clip,
                   const PaintChunk& chunk)
        : bounds_in_layer(bounds),
          chunk_to_layer_transform(chunk_to_layer_transform),
          chunk_to_layer_clip(chunk_to_layer_clip),
          id(chunk.id),
          is_cacheable(chunk.is_cacheable),
          properties(chunk.properties) {}

    bool Matches(const PaintChunk& new_chunk) const {
      return is_cacheable && new_chunk.Matches(id);
    }

    IntRect bounds_in_layer;
    TransformationMatrix chunk_to_layer_transform;
    FloatClipRect chunk_to_layer_clip;
    PaintChunk::Id id;
    bool is_cacheable;
    PaintChunkProperties properties;
  };

  IntRect MapRectFromChunkToLayer(const FloatRect&, const PaintChunk&) const;
  TransformationMatrix ChunkToLayerTransform(const PaintChunk&) const;
  FloatClipRect ChunkToLayerClip(const PaintChunk&) const;

  void GenerateRasterInvalidations(
      const Vector<const PaintChunk*>& new_chunks,
      const Vector<PaintChunkInfo>& new_chunks_info);
  size_t MatchNewChunkToOldChunk(const PaintChunk& new_chunk, size_t old_index);
  void AddDisplayItemRasterInvalidations(const PaintChunk&);
  void IncrementallyInvalidateChunk(const PaintChunkInfo& old_chunk,
                                    const PaintChunkInfo& new_chunk);
  void FullyInvalidateChunk(const PaintChunkInfo& old_chunk,
                            const PaintChunkInfo& new_chunk,
                            PaintInvalidationReason);
  ALWAYS_INLINE void FullyInvalidateNewChunk(const PaintChunkInfo&,
                                             PaintInvalidationReason);
  ALWAYS_INLINE void FullyInvalidateOldChunk(const PaintChunkInfo&,
                                             PaintInvalidationReason);
  ALWAYS_INLINE void AddRasterInvalidation(const IntRect&,
                                           const DisplayItemClient*,
                                           PaintInvalidationReason,
                                           const String* debug_name = nullptr);
  PaintInvalidationReason ChunkPropertiesChanged(
      const PaintChunkInfo& new_chunk,
      const PaintChunkInfo& old_chunk) const;

  // Clip a rect in the layer space by the layer bounds.
  template <typename Rect>
  Rect ClipByLayerBounds(const Rect& r) const {
    return Intersection(
        r, Rect(0, 0, layer_bounds_.width(), layer_bounds_.height()));
  }

  RasterInvalidationFunction raster_invalidation_function_;
  gfx::Rect layer_bounds_;
  PropertyTreeState layer_state_;
  Vector<PaintChunkInfo> paint_chunks_info_;

  struct RasterInvalidationTrackingInfo {
    using ClientDebugNamesMap = HashMap<const DisplayItemClient*, String>;
    ClientDebugNamesMap new_client_debug_names;
    ClientDebugNamesMap old_client_debug_names;
    RasterInvalidationTracking tracking;
  };
  std::unique_ptr<RasterInvalidationTrackingInfo> tracking_info_;
};

}  // namespace blink

#endif  // CompositedLayerRasterInvalidator_h
