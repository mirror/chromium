// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_PICTURE_LAYER_IMPL_H_
#define CC_LAYERS_PICTURE_LAYER_IMPL_H_

#include <set>
#include <string>
#include <vector>

#include "cc/base/cc_export.h"
#include "cc/base/scoped_ptr_vector.h"
#include "cc/layers/layer_impl.h"
#include "cc/resources/picture_layer_tiling.h"
#include "cc/resources/picture_layer_tiling_set.h"
#include "cc/resources/picture_pile_impl.h"
#include "cc/resources/tiling_set_eviction_queue.h"
#include "cc/resources/tiling_set_raster_queue.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace cc {

struct AppendQuadsData;
class MicroBenchmarkImpl;
class Tile;

class CC_EXPORT PictureLayerImpl
    : public LayerImpl,
      NON_EXPORTED_BASE(public PictureLayerTilingClient) {
 public:
  struct CC_EXPORT Pair {
    Pair();
    Pair(PictureLayerImpl* active_layer, PictureLayerImpl* pending_layer);
    ~Pair();

    PictureLayerImpl* active;
    PictureLayerImpl* pending;
  };

  static scoped_ptr<PictureLayerImpl> Create(LayerTreeImpl* tree_impl,
                                             int id,
                                             bool is_mask) {
    return make_scoped_ptr(new PictureLayerImpl(tree_impl, id, is_mask));
  }
  ~PictureLayerImpl() override;

  bool is_mask() const { return is_mask_; }

  scoped_ptr<TilingSetEvictionQueue> CreateEvictionQueue(
      TreePriority tree_priority);
  scoped_ptr<TilingSetRasterQueue> CreateRasterQueue(bool prioritize_low_res);

  // LayerImpl overrides.
  const char* LayerTypeAsString() const override;
  scoped_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;
  void PushPropertiesTo(LayerImpl* layer) override;
  void AppendQuads(RenderPass* render_pass,
                   const Occlusion& occlusion_in_content_space,
                   AppendQuadsData* append_quads_data) override;
  bool UpdateTiles(const Occlusion& occlusion_in_content_space,
                   bool resourceless_software_draw) override;
  void NotifyTileStateChanged(const Tile* tile) override;
  void DidBeginTracing() override;
  void ReleaseResources() override;
  skia::RefPtr<SkPicture> GetPicture() override;

  // PictureLayerTilingClient overrides.
  scoped_refptr<Tile> CreateTile(float contents_scale,
                                 const gfx::Rect& content_rect) override;
  gfx::Size CalculateTileSize(const gfx::Size& content_bounds) const override;
  const Region* GetPendingInvalidation() override;
  const PictureLayerTiling* GetPendingOrActiveTwinTiling(
      const PictureLayerTiling* tiling) const override;
  PictureLayerTiling* GetRecycledTwinTiling(
      const PictureLayerTiling* tiling) override;
  TilePriority::PriorityBin GetMaxTilePriorityBin() const override;
  WhichTree GetTree() const override;
  bool RequiresHighResToDraw() const override;

  void UpdateRasterSource(scoped_refptr<RasterSource> raster_source,
                          Region* new_invalidation,
                          const PictureLayerTilingSet* pending_set);

  // Mask-related functions.
  void GetContentsResourceId(ResourceProvider::ResourceId* resource_id,
                             gfx::Size* resource_size) const override;

  void SetNearestNeighbor(bool nearest_neighbor);

  size_t GPUMemoryUsageInBytes() const override;

  void RunMicroBenchmark(MicroBenchmarkImpl* benchmark) override;

  bool CanHaveTilings() const;

  // Functions used by tile manager.
  PictureLayerImpl* GetPendingOrActiveTwinLayer() const;
  bool IsOnActiveOrPendingTree() const;
  // Virtual for testing.
  virtual bool HasValidTilePriorities() const;
  bool AllTilesRequiredForActivationAreReadyToDraw() const;
  bool AllTilesRequiredForDrawAreReadyToDraw() const;

  // Used for benchmarking
  RasterSource* GetRasterSource() const { return raster_source_.get(); }

 protected:
  friend class LayerRasterTileIterator;
  using TileRequirementCheck = bool (PictureLayerTiling::*)(const Tile*) const;

  PictureLayerImpl(LayerTreeImpl* tree_impl, int id, bool is_mask);
  PictureLayerTiling* AddTiling(float contents_scale);
  void RemoveAllTilings();
  void AddTilingsForRasterScale();
  bool UpdateTilePriorities(const Occlusion& occlusion_in_content_space);
  virtual bool ShouldAdjustRasterScale() const;
  virtual void RecalculateRasterScales();
  void CleanUpTilingsOnActiveLayer(
      std::vector<PictureLayerTiling*> used_tilings);
  float MinimumContentsScale() const;
  float MaximumContentsScale() const;
  void ResetRasterScale();
  void UpdateViewportRectForTilePriorityInContentSpace();
  PictureLayerImpl* GetRecycledTwinLayer() const;

  void SanityCheckTilingState() const;
  // Checks if all tiles required for a certain action (e.g. activation) are
  // ready to draw.  is_tile_required_callback gets called on all candidate
  // tiles and returns true if the tile is required for the action.
  bool AllTilesRequiredAreReadyToDraw(
      TileRequirementCheck is_tile_required_callback) const;

  bool ShouldAdjustRasterScaleDuringScaleAnimations() const;

  void GetDebugBorderProperties(SkColor* color, float* width) const override;
  void GetAllTilesForTracing(std::set<const Tile*>* tiles) const override;
  void AsValueInto(base::debug::TracedValue* dict) const override;

  virtual void UpdateIdealScales();
  float MaximumTilingContentsScale() const;
  scoped_ptr<PictureLayerTilingSet> CreatePictureLayerTilingSet();

  PictureLayerImpl* twin_layer_;

  scoped_ptr<PictureLayerTilingSet> tilings_;
  scoped_refptr<RasterSource> raster_source_;
  Region invalidation_;

  float ideal_page_scale_;
  float ideal_device_scale_;
  float ideal_source_scale_;
  float ideal_contents_scale_;

  float raster_page_scale_;
  float raster_device_scale_;
  float raster_source_scale_;
  float raster_contents_scale_;
  float low_res_raster_contents_scale_;

  bool raster_source_scale_is_fixed_;
  bool was_screen_space_transform_animating_;
  // A sanity state check to make sure UpdateTilePriorities only gets called
  // after a CalculateContentsScale/ManageTilings.
  bool should_update_tile_priorities_;
  bool only_used_low_res_last_append_quads_;
  const bool is_mask_;

  bool nearest_neighbor_;

  // Any draw properties derived from |transform|, |viewport|, and |clip|
  // parameters in LayerTreeHostImpl::SetExternalDrawConstraints are not valid
  // for prioritizing tiles during resourceless software draws. This is because
  // resourceless software draws can have wildly different transforms/viewports
  // from regular draws. Save a copy of the required draw properties of the last
  // frame that has a valid viewport for prioritizing tiles.
  gfx::Rect visible_rect_for_tile_priority_;
  gfx::Rect viewport_rect_for_tile_priority_in_content_space_;

  DISALLOW_COPY_AND_ASSIGN(PictureLayerImpl);
};

}  // namespace cc

#endif  // CC_LAYERS_PICTURE_LAYER_IMPL_H_
