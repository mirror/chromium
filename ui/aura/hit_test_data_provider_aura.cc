// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/hit_test_data_provider_aura.h"

#include "base/containers/adapters.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "ui/aura/window.h"

namespace {

viz::mojom::HitTestRegionPtr CreateHitTestRegion(
    const aura::Window* window,
    uint32_t child_count,
    std::vector<viz::mojom::HitTestRectPtr> hit_test_rects) {
  const ui::Layer* layer = window->layer();
  DCHECK(layer);

  auto hit_test_region = viz::mojom::HitTestRegion::New();
  DCHECK(window->GetFrameSinkId().is_valid());
  hit_test_region->frame_sink_id = window->GetFrameSinkId();
  // Checking |layer| may not be correct, since the actual layer that embeds
  // the surface may be a descendent of |layer|, instead of |layer| itself.
  if (window->IsEmbeddingClient()) {
    DCHECK(window->GetLocalSurfaceId().is_valid());
    hit_test_region->local_surface_id = window->GetLocalSurfaceId();
    hit_test_region->flags = viz::mojom::kHitTestChildSurface;
  } else {
    hit_test_region->flags = viz::mojom::kHitTestMine;
  }
  hit_test_region->transform = layer->transform();
  hit_test_region->rects = std::move(hit_test_rects);
  hit_test_region->child_count = child_count;

  return hit_test_region;
}

viz::mojom::HitTestRectPtr CreateHitTestRect(uint32_t flags,
                                             const gfx::Rect& rect) {
  auto hit_test_rect = viz::mojom::HitTestRect::New();
  hit_test_rect->flags = flags;
  hit_test_rect->rect = rect;
  return hit_test_rect;
}

}  // namespace

namespace aura {

HitTestDataProviderAura::HitTestDataProviderAura(aura::Window* window)
    : window_(window) {}

HitTestDataProviderAura::~HitTestDataProviderAura() {}

viz::mojom::HitTestRegionListPtr HitTestDataProviderAura::GetHitTestData()
    const {
  const ui::mojom::EventTargetingPolicy event_targeting_policy =
      window_->event_targeting_policy();
  if (!window_->IsVisible() ||
      event_targeting_policy == ui::mojom::EventTargetingPolicy::NONE)
    return nullptr;

  auto hit_test_region_list = viz::mojom::HitTestRegionList::New();
  hit_test_region_list->flags =
      event_targeting_policy ==
              ui::mojom::EventTargetingPolicy::DESCENDANTS_ONLY
          ? viz::mojom::kHitTestIgnore
          : viz::mojom::kHitTestMine;
  hit_test_region_list->bounds = window_->bounds();

  GetHitTestDataRecursively(window_, hit_test_region_list.get(), nullptr);
  return hit_test_region_list;
}

uint32_t HitTestDataProviderAura::GetHitTestDataRecursively(
    aura::Window* window,
    viz::mojom::HitTestRegionList* hit_test_region_list,
    WindowTargeter* parent_targeter) const {
  uint32_t child_count = 0;
  if (!window->IsEmbeddingClient()) {
    // TODO(varkha): Figure out if we need to add hit-test regions for |window|.
    // Walk the children in Z-order (reversed order of children()) to produce
    // the hit-test data. Each child's hit test data is added before the
    // hit-test data from the child's descendants because the child could clip
    // its descendants for the purpose of event handling.

    for (aura::Window* child : window->children()) {
      const ui::mojom::EventTargetingPolicy event_targeting_policy =
          child->event_targeting_policy();
      if (!child->IsVisible() ||
          event_targeting_policy == ui::mojom::EventTargetingPolicy::NONE)
        continue;

      if (event_targeting_policy !=
          ui::mojom::EventTargetingPolicy::TARGET_ONLY) {
        child_count += GetHitTestDataRecursively(
            child, hit_test_region_list,
            static_cast<WindowTargeter*>(window->GetEventTargeter()));
      }
    }
  }

  if (window->GetFrameSinkId() != window_->GetFrameSinkId() &&
      window->event_targeting_policy() !=
          ui::mojom::EventTargetingPolicy::DESCENDANTS_ONLY) {
    std::vector<viz::mojom::HitTestRectPtr> hit_test_rects;
    gfx::Rect rect_mouse(window->bounds());
    gfx::Rect rect_touch;
    bool touch_and_mouse_are_same = true;
    WindowTargeter* targeter =
        static_cast<WindowTargeter*>(window->GetEventTargeter());
    if (!targeter)
      targeter = parent_targeter;
    // Use the |child|'s (when set) or the |window|'s |targeter| to query for
    // possibly expanded hit-test area. Use the |child| bounds with mouse and
    // touch flags when there is no |targeter|.
    if (targeter &&
        targeter->GetHitTestRects(window, &rect_mouse, &rect_touch)) {
      touch_and_mouse_are_same = rect_mouse == rect_touch;
    }

    auto shape_rects =
        targeter ? targeter->GetExtraHitTestShapeRects(window) : nullptr;
    if (shape_rects) {
      // The |child| has a complex shape. Clip it to |rect_mouse|.
      const gfx::Vector2d offset = window->bounds().OffsetFromOrigin();
      for (const gfx::Rect& shape_rect : *shape_rects) {
        gfx::Rect rect = shape_rect;
        rect.Offset(offset);
        rect.Intersect(rect_mouse);
        if (rect.IsEmpty())
          continue;
        hit_test_rects.push_back(CreateHitTestRect(
            viz::mojom::kHitTestMouse | viz::mojom::kHitTestTouch, rect));
      }
    } else {
      // The |child| has possibly same mouse and touch hit-test areas.
      if (!rect_mouse.IsEmpty()) {
        hit_test_rects.push_back(CreateHitTestRect(
            touch_and_mouse_are_same
                ? (viz::mojom::kHitTestMouse | viz::mojom::kHitTestTouch)
                : viz::mojom::kHitTestMouse,
            rect_mouse));
      }
      if (!touch_and_mouse_are_same && !rect_touch.IsEmpty()) {
        hit_test_rects.push_back(
            CreateHitTestRect(viz::mojom::kHitTestTouch, rect_touch));
      }
    }
    if (hit_test_rects.size()) {
      hit_test_region_list->regions.insert(
          hit_test_region_list->regions.begin(),
          CreateHitTestRegion(window, child_count, std::move(hit_test_rects)));
      child_count++;
    }
  }
  return child_count;
}

}  // namespace aura
