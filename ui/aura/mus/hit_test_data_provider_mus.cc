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

std::unique_ptr<viz::HitTestDataProvider::HitTestRegionList>
HitTestDataProviderMus::GetHitTestData() {
  auto hit_test_region_list = base::MakeUnique<HitTestRegionList>();
  GetHitTestDataRecursively(window_, hit_test_region_list.get());
  return hit_test_region_list;
}

void HitTestDataProviderMus::GetHitTestDataRecursively(
    aura::Window* window,
    HitTestRegionList* hit_test_region_list) {
  WindowTargeter* targeter =
      static_cast<WindowTargeter*>(window->GetEventTargeter());

  // Walk the children in Z-order (reversed order of children()) to produce
  // the hit-test data. Each child's hit test data is added before the hit-test
  // data from the child's descendants because the child could clip its
  // descendants for the purpose of event handling.
  for (aura::Window* child : base::Reversed(window->children())) {
    gfx::Rect rect_mouse;
    gfx::Rect rect_touch;
    if (targeter &&
        targeter->GetHitTestRects(child, &rect_mouse, &rect_touch)) {
      const bool touch_and_mouse_are_same = rect_mouse == rect_touch;
      const WindowPortMus* window_port = WindowPortMus::Get(child);
      viz::AggregatedHitTestRegion hit_test_region = {
          window_port->frame_sink_id(), window_port->server_id(), 0};
      if (!rect_mouse.IsEmpty()) {
        hit_test_region.flags = touch_and_mouse_are_same ? 0x03 : 0x01;
        hit_test_region.rect = rect_mouse;
        hit_test_region_list->push_back(hit_test_region);
      }
      if (!touch_and_mouse_are_same && !rect_touch.IsEmpty()) {
        hit_test_region.flags = 0x02;
        hit_test_region.rect = rect_touch;
        hit_test_region_list->push_back(hit_test_region);
      }
    }
    GetHitTestDataRecursively(child, hit_test_region_list);
  }
}

}  // namespace aura