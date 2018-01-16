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
  } else if (surface_id.local_surface_id().is_valid()) {
    hit_test_region->local_surface_id = surface_id.local_surface_id();
    hit_test_region->flags = flags | viz::mojom::kHitTestChildSurface;
  } else {
    hit_test_region->flags = flags | viz::mojom::kHitTestMine;
  }

  hit_test_region->rect = rect;
  hit_test_region->transform = layer->ScreenSpaceTransform();

  return hit_test_region;
}

}  // namespace

namespace viz {

HitTestDataProviderContent::HitTestDataProviderContent(
    const cc::LayerTreeHostImpl* host_impl)
    : host_impl_(host_impl) {}

HitTestDataProviderContent::~HitTestDataProviderContent() {}

mojom::HitTestRegionListPtr HitTestDataProviderContent::GetHitTestData() const {
  auto hit_test_region_list = mojom::HitTestRegionList::New();
  hit_test_region_list->flags = mojom::kHitTestMine;
  GetHitTestDataRecursively(host_impl_, hit_test_region_list.get());
  LOG(ERROR) << "HitTestRegionList: (viewport = "
             << host_impl_->DeviceViewport().ToString() << ")";
  for (auto& region_ptr : hit_test_region_list->regions) {
    LOG(ERROR) << "  region " << region_ptr->rect.ToString();
  }
  return hit_test_region_list;
}

void HitTestDataProviderContent::GetHitTestDataRecursively(
    const cc::LayerTreeHostImpl* host_impl,
    mojom::HitTestRegionList* hit_test_region_list) const {
  hit_test_region_list->bounds = host_impl->DeviceViewport();
  for (const auto* layer : *host_impl->active_tree()) {
    if (layer->is_surface_layer()) {
      gfx::Rect content_rect(layer->bounds());
      hit_test_region_list->regions.insert(
          hit_test_region_list->regions.begin(),
          CreateHitTestRegion(static_cast<const cc::SurfaceLayerImpl*>(layer),
                              mojom::kHitTestMouse | mojom::kHitTestTouch,
                              content_rect));
    }
  }
}

}  // namespace viz
