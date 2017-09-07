// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/unified_mouse_warp_controller.h"

#include <cmath>

#include <queue>
#include <set>

#include "ash/display/display_util.h"
#include "ash/display/mirror_window_controller.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/host/ash_window_tree_host.h"
#include "ash/shell.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/layout.h"
#include "ui/display/display_finder.h"
#include "ui/display/display_layout.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/display_manager_utilities.h"
#include "ui/display/screen.h"
#include "ui/events/event_utils.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

namespace {

AshWindowTreeHost* GetMirroringAshWindowTreeHostForDisplayId(
    int64_t display_id) {
  return Shell::Get()
      ->window_tree_host_manager()
      ->mirror_window_controller()
      ->GetAshWindowTreeHostForDisplayId(display_id);
}

// Find a WindowTreeHost used for mirroring displays that contains
// the |point_in_screen|. Returns nullptr if such WTH does not exist.
aura::WindowTreeHost* FindMirroringWindowTreeHostFromScreenPoint(
    const gfx::Point& point_in_screen) {
  display::Displays mirroring_display_list =
      Shell::Get()->display_manager()->software_mirroring_display_list();
  auto iter = display::FindDisplayContainingPoint(mirroring_display_list,
                                                  point_in_screen);
  if (iter == mirroring_display_list.end())
    return nullptr;
  return GetMirroringAshWindowTreeHostForDisplayId(iter->id())
      ->AsWindowTreeHost();
}

}  // namespace

UnifiedMouseWarpController::UnifiedMouseWarpController()
    : current_cursor_display_id_(display::kInvalidDisplayId),
      update_location_for_test_(false),
      bounds_computed_(false) {}

UnifiedMouseWarpController::~UnifiedMouseWarpController() {}

bool UnifiedMouseWarpController::WarpMouseCursor(ui::MouseEvent* event) {
  // Mirroring windows are created asynchronously, so compute the edge
  // beounds when we received an event instead of in constructor.
  if (!bounds_computed_)
    ComputeBounds();

  aura::Window* target = static_cast<aura::Window*>(event->target());
  gfx::Point point_in_unified_host = event->location();
  ::wm::ConvertPointToScreen(target, &point_in_unified_host);
  // The display bounds of the mirroring windows isn't scaled, so
  // transform back to the host coordinates.
  target->GetHost()->GetRootTransform().TransformPoint(&point_in_unified_host);

  if (current_cursor_display_id_ != display::kInvalidDisplayId) {
    aura::client::CursorClient* cursor_client =
        aura::client::GetCursorClient(target->GetRootWindow());
    if (cursor_client) {
      display::Displays mirroring_display_list =
          Shell::Get()->display_manager()->software_mirroring_display_list();
      auto iter = display::FindDisplayContainingPoint(mirroring_display_list,
                                                      point_in_unified_host);
      if (iter != mirroring_display_list.end() &&
          current_cursor_display_id_ != iter->id()) {
        cursor_client->SetDisplay(*iter);
        current_cursor_display_id_ = display::kInvalidDisplayId;
      }
    }
  }

  // A native event may not exist in unit test.
  if (!event->HasNativeEvent())
    return false;

  gfx::Point point_in_native =
      ui::EventSystemLocationFromNative(event->native_event());

  // TODO(dnicoara): crbug.com/415680 Move cursor warping into Ozone once Ozone
  // has access to the logical display layout.
  // Native events in Ozone are in the native window coordinate system. We need
  // to translate them to get the global position.
  aura::WindowTreeHost* host =
      FindMirroringWindowTreeHostFromScreenPoint(point_in_unified_host);
  if (!host)
    return false;
  point_in_native.Offset(host->GetBoundsInPixels().x(),
                         host->GetBoundsInPixels().y());

  return WarpMouseCursorInNativeCoords(point_in_native, point_in_unified_host,
                                       update_location_for_test_);
}

void UnifiedMouseWarpController::SetEnabled(bool enabled) {
  // Mouse warp shuld be always on in Unified mode.
}

void UnifiedMouseWarpController::ComputeBounds() {
  display::Displays display_list =
      Shell::Get()->display_manager()->software_mirroring_display_list();

  if (display_list.size() < 2) {
    LOG(ERROR) << "Mirroring Display lost during re-configuration";
    return;
  }

  for (size_t i = 0; i < display_list.size() - 1; ++i) {
    const display::Display& first = display_list[i];
    for (size_t j = i + 1; j < display_list.size(); ++j) {
      const display::Display& second = display_list[j];
      gfx::Rect first_edge;
      gfx::Rect second_edge;
      if (display::ComputeBoundary(first, second, &first_edge, &second_edge)) {
        first_edge = GetNativeEdgeBounds(
            GetMirroringAshWindowTreeHostForDisplayId(first.id()), first_edge);
        second_edge = GetNativeEdgeBounds(
            GetMirroringAshWindowTreeHostForDisplayId(second.id()),
            second_edge);

        displays_edges_map_[first.id()].emplace_back(first.id(), second.id(),
                                                     first_edge);
        displays_edges_map_[second.id()].emplace_back(second.id(), first.id(),
                                                      second_edge);
      }
    }
  }

  bounds_computed_ = true;
}

bool UnifiedMouseWarpController::WarpMouseCursorInNativeCoords(
    const gfx::Point& point_in_native,
    const gfx::Point& point_in_unified_host,
    bool update_mouse_location_now) {
  display::Displays mirroring_display_list =
      Shell::Get()->display_manager()->software_mirroring_display_list();
  const auto iter = display::FindDisplayContainingPoint(mirroring_display_list,
                                                        point_in_unified_host);
  if (iter == mirroring_display_list.end())
    return false;

  const auto edges_iter = displays_edges_map_.find(iter->id());
  if (edges_iter == displays_edges_map_.end())
    return false;

  const std::vector<DisplayEdge>& potential_edges = edges_iter->second;
  for (const auto& edge : potential_edges) {
    if (edge.edge_native_bounds_in_source_display.Contains(point_in_native)) {
      // Wait updating the cursor until the cursor moves to the new display
      // to avoid showing the wrong sized cursor at the source display.
      current_cursor_display_id_ = edge.source_display_id;

      AshWindowTreeHost* target_ash_host =
          GetMirroringAshWindowTreeHostForDisplayId(edge.target_display_id);
      MoveCursorTo(target_ash_host, point_in_unified_host,
                   update_mouse_location_now);
      return true;
    }
  }

  return false;
}

}  // namespace ash
