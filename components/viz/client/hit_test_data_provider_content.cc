// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hit_test_data_provider_content.h"

#include "cc/layers/surface_layer_impl.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_impl.h"

namespace {

viz::mojom::HitTestRegionPtr CreateHitTestRegion(
    const cc::SurfaceLayerImpl* layer,
    uint32_t flags,
    const gfx::Rect& rect) {
  auto hit_test_region = viz::mojom::HitTestRegion::New();
  const auto& surface_id = layer->primary_surface_id();
  hit_test_region->frame_sink_id = surface_id.frame_sink_id();
  if (layer->is_clipped()) {
    hit_test_region->flags = viz::mojom::kHitTestAsk;
  }
  if (surface_id.local_surface_id().is_valid()) {
    hit_test_region->local_surface_id = surface_id.local_surface_id();
    hit_test_region->flags = flags | viz::mojom::kHitTestChildSurface;
  } else {
    hit_test_region->flags = flags | viz::mojom::kHitTestMine;
  }

  hit_test_region->rect = rect;
  if (!layer->RootSpaceTransform().GetInverse(&(hit_test_region->transform)))
    hit_test_region->transform = gfx::Transform();

  return hit_test_region;
}

}  // namespace

namespace viz {

HitTestDataProviderContent::HitTestDataProviderContent(
    const cc::LayerTreeHostImpl* host_impl)
    : host_impl_(host_impl) {}

HitTestDataProviderContent::~HitTestDataProviderContent() {}

mojom::HitTestRegionListPtr HitTestDataProviderContent::GetHitTestData(
    const CompositorFrame& compositor_frame) const {
  auto hit_test_region_list = mojom::HitTestRegionList::New();
  hit_test_region_list->flags = mojom::kHitTestMine;
  hit_test_region_list->bounds = host_impl_->DeviceViewport();
  hit_test_region_list->transform = host_impl_->DrawTransform();
  for (const auto* layer : *host_impl_->active_tree()) {
    if (layer->is_surface_layer()) {
      gfx::Rect content_rect(layer->drawable_content_rect().size());
      if (content_rect.IsEmpty())
        continue;
      hit_test_region_list->regions.insert(
          hit_test_region_list->regions.begin(),
          CreateHitTestRegion(static_cast<const cc::SurfaceLayerImpl*>(layer),
                              mojom::kHitTestMouse | mojom::kHitTestTouch,
                              content_rect));
    }
  }
  return hit_test_region_list;
}

}  // namespace viz
