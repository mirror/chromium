// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/hit_test_data_provider_mus.h"

#include "base/containers/adapters.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"

namespace aura {

HitTestDataProviderMus::HitTestDataProviderMus(aura::Window* window)
    : window_(window) {}

HitTestDataProviderMus::~HitTestDataProviderMus() {}

viz::mojom::HitTestRegionListPtr HitTestDataProviderMus::GetHitTestData()
    const {
  auto hit_test_region_list = viz::mojom::HitTestRegionList::New();
  GetHitTestDataRecursively(window_, hit_test_region_list.get());
  return hit_test_region_list;
}

void HitTestDataProviderMus::GetHitTestDataRecursively(
    aura::Window* window,
    viz::mojom::HitTestRegionList* hit_test_region_list) const {
  WindowTargeter* targeter =
      static_cast<WindowTargeter*>(window->GetEventTargeter());

  // Walk the children in Z-order (reversed order of children()) to produce
  // the hit-test data. Each child's hit test data is added before the hit-test
  // data from the child's descendants because the child could clip its
  // descendants for the purpose of event handling.
  for (aura::Window* child : base::Reversed(window->children())) {
    gfx::Rect rect_mouse(child->bounds());
    gfx::Rect rect_touch;
    bool touch_and_mouse_are_same = true;
    const WindowMus* window_mus = WindowMus::Get(child);
    const WindowPortMus* window_port = WindowPortMus::Get(child);
    viz::SurfaceId surface_id(window_port->frame_sink_id(),
                              window_mus->GetLocalSurfaceId());
    uint32_t flags = window_port->client_surface_embedder()
                         ? viz::mojom::kHitTestChildSurface
                         : 0;
    if (targeter &&
        targeter->GetHitTestRects(child, &rect_mouse, &rect_touch)) {
      touch_and_mouse_are_same = rect_mouse == rect_touch;
    }
    if (!rect_mouse.IsEmpty()) {
      auto hit_test_region = viz::mojom::HitTestRegion::New();
      hit_test_region->surface_id = surface_id;
      hit_test_region->window_id = window_mus->server_id();
      hit_test_region->flags =
          flags | (touch_and_mouse_are_same
                       ? (viz::mojom::kHitTestMouse | viz::mojom::kHitTestTouch)
                       : viz::mojom::kHitTestMouse);
      hit_test_region->rect = rect_mouse;
      if (child->layer())
        hit_test_region->transform = child->layer()->transform();
      hit_test_region_list->regions.push_back(std::move(hit_test_region));
    }
    if (!touch_and_mouse_are_same && !rect_touch.IsEmpty()) {
      auto hit_test_region = viz::mojom::HitTestRegion::New();
      hit_test_region->surface_id = surface_id;
      hit_test_region->window_id = window_mus->server_id();
      hit_test_region->flags = flags | viz::mojom::kHitTestTouch;
      hit_test_region->rect = rect_touch;
      if (child->layer())
        hit_test_region->transform = child->layer()->transform();
      hit_test_region_list->regions.push_back(std::move(hit_test_region));
    }
    GetHitTestDataRecursively(child, hit_test_region_list);
  }
}

}  // namespace aura