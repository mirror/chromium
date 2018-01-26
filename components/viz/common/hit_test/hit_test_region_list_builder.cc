// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/hit_test/hit_test_region_list_builder.h"

#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/common/surfaces/surface_id.h"

namespace viz {

// static
mojom::HitTestRegionListPtr HitTestRegionListBuilder::CreateHitTestData(
    const CompositorFrame& frame) {
  auto hit_test_region_list = mojom::HitTestRegionList::New();
  hit_test_region_list->flags = mojom::kHitTestMine;
  hit_test_region_list->bounds.set_size(frame.size_in_pixels());

  for (const auto& render_pass : frame.render_pass_list) {
    for (const DrawQuad* quad : render_pass->quad_list) {
      if (quad->material == DrawQuad::SURFACE_CONTENT) {
        const SurfaceDrawQuad* surface_quad =
            SurfaceDrawQuad::MaterialCast(quad);
        auto hit_test_region = mojom::HitTestRegion::New();
        const SurfaceId& surface_id = surface_quad->primary_surface_id;
        hit_test_region->frame_sink_id = surface_id.frame_sink_id();
        hit_test_region->local_surface_id = surface_id.local_surface_id();
        hit_test_region->flags = mojom::kHitTestChildSurface;
        hit_test_region->rect = surface_quad->rect;
        hit_test_region_list->regions.push_back(std::move(hit_test_region));
      }
    }
  }
  return hit_test_region_list;
}

}  // namespace viz
