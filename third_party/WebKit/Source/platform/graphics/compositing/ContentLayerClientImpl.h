// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ContentLayerClientImpl_h
#define ContentLayerClientImpl_h

#include "cc/layers/content_layer_client.h"
#include "cc/layers/picture_layer.h"
#include "platform/PlatformExport.h"
#include "platform/graphics/GraphicsLayerClient.h"
#include "platform/graphics/paint/PaintChunk.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/Vector.h"

namespace blink {

class JSONArray;
class JSONObject;
class PaintArtifact;
struct RasterInvalidationTracking;

class PLATFORM_EXPORT ContentLayerClientImpl : public cc::ContentLayerClient {
  WTF_MAKE_NONCOPYABLE(ContentLayerClientImpl);
  USING_FAST_MALLOC(ContentLayerClientImpl);

 public:
  ContentLayerClientImpl()
      : cc_picture_layer_(cc::PictureLayer::Create(this)) {}
  ~ContentLayerClientImpl() override {}

  // cc::ContentLayerClient
  gfx::Rect PaintableRegion() override {
    return gfx::Rect(layer_bounds_.size());
  }
  scoped_refptr<cc::DisplayItemList> PaintContentsToDisplayList(
      PaintingControlSetting) override {
    return cc_display_item_list_;
  }
  bool FillsBoundsCompletely() const override { return false; }
  size_t GetApproximateUnsharedMemoryUsage() const override {
    // TODO(jbroman): Actually calculate memory usage.
    return 0;
  }

  void SetTracksRasterInvalidations(bool);

  std::unique_ptr<JSONObject> LayerAsJSON(LayerTreeFlags);

  scoped_refptr<cc::PictureLayer> UpdateCcPictureLayer(
      const PaintArtifact&,
      const FloatRect& bounds,
      const Vector<const PaintChunk*>&,
      gfx::Vector2dF& layer_offset,
      bool store_debug_info);

  bool Matches(const PaintChunk& paint_chunk) {
    return paint_chunks_info_.size() &&
           paint_chunk.Matches(paint_chunks_info_[0].id);
  }

 private:
  struct PaintChunkInfo {
    IntRect bounds_in_layer;
    Optional<PaintChunk::Id> id;
  };

  IntRect MapRasterInvalidationRectFromChunkToLayer(
      const FloatRect&,
      const PaintChunk&,
      const PaintChunk& first_chunk) const;

  void GenerateRasterInvalidations(const Vector<const PaintChunk*>&);
  void UpdatePaintChunkInfo(const PaintChunk& new_chunk,
                            const PaintChunk& first_chunk,
                            PaintChunkInfo&) const;
  void AddDisplayItemRasterInvalidations(const PaintChunk&,
                                         const PaintChunk& first_chunk);
  void InvalidateRasterForChunk(const PaintChunkInfo&, PaintInvalidationReason);
  void InvalidateRasterForWholeLayer();

  scoped_refptr<cc::PictureLayer> cc_picture_layer_;
  scoped_refptr<cc::DisplayItemList> cc_display_item_list_;
  gfx::Rect layer_bounds_;

  Vector<PaintChunkInfo> paint_chunks_info_;
  Vector<std::unique_ptr<JSONArray>> paint_chunk_debug_data_;
  String debug_name_;

  struct RasterInvalidationTrackingInfo {
    using ClientDebugNamesMap = HashMap<const DisplayItemClient*, String>;
    ClientDebugNamesMap new_client_debug_names;
    ClientDebugNamesMap old_client_debug_names;
    RasterInvalidationTracking tracking;
  };
  std::unique_ptr<RasterInvalidationTrackingInfo>
      raster_invalidation_tracking_info_;
};

}  // namespace blink

#endif  // ContentLayerClientImpl_h
