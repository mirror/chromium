// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/hit_test_data_provider_draw_quad_renderer.h"

#include "components/viz/common/quads/surface_draw_quad.h"

namespace viz {

HitTestDataProviderDrawQuadRenderer::HitTestDataProviderDrawQuadRenderer() =
    default;
HitTestDataProviderDrawQuadRenderer::~HitTestDataProviderDrawQuadRenderer() =
    default;

mojom::HitTestRegionListPtr HitTestDataProviderDrawQuadRenderer::GetHitTestData(
    const CompositorFrame& compositor_frame) const {
  // Derive hit test regions from information in the CompositorFrame.
  auto hit_test_region_list = mojom::HitTestRegionList::New();
  hit_test_region_list->flags =
      mojom::kHitTestMouse | mojom::kHitTestTouch | mojom::kHitTestMine;
  hit_test_region_list->bounds.set_size(compositor_frame.size_in_pixels());

  for (const auto& render_pass : compositor_frame.render_pass_list) {
    for (const DrawQuad* quad : render_pass->quad_list) {
      if (quad->material == DrawQuad::SURFACE_CONTENT) {
        const SurfaceDrawQuad* surface_quad =
            SurfaceDrawQuad::MaterialCast(quad);
        auto hit_test_region = mojom::HitTestRegion::New();
        const SurfaceId& surface_id = surface_quad->primary_surface_id;
        hit_test_region->frame_sink_id = surface_id.frame_sink_id();
        hit_test_region->local_surface_id = surface_id.local_surface_id();
        hit_test_region->flags =
            mojom::kHitTestChildSurface | mojom::kHitTestAsk;
        hit_test_region->rect = surface_quad->rect;
        gfx::Transform target_to_quad_transform;
        bool invertible =
            surface_quad->shared_quad_state->quad_to_target_transform
                .GetInverse(&target_to_quad_transform);
        DCHECK(invertible);
        hit_test_region->transform = target_to_quad_transform;
        hit_test_region_list->regions.push_back(std::move(hit_test_region));
      }
    }
  }

  return hit_test_region_list;
}

}  // namespace viz
