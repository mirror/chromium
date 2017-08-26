// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/unified_mouse_warp_controller.h"

#include <cmath>

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

#include "my_out.h"

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
      update_location_for_test_(false) {}

UnifiedMouseWarpController::~UnifiedMouseWarpController() {}

bool UnifiedMouseWarpController::WarpMouseCursor(ui::MouseEvent* event) {
  auto marker = MARK_FUNC();
  D_OUT(marker, "################################################");
  D_OUT(marker, "################################################");
  // Mirroring windows are created asynchronously, so compute the edge
  // beounds when we received an event instead of in constructor.
  if (first_edge_bounds_in_native_.IsEmpty())
    ComputeBounds();

  aura::Window* target = static_cast<aura::Window*>(event->target());
  gfx::Point point_in_unified_host = event->location();
  D_OUT_VAL(marker, point_in_unified_host.ToString());
  ::wm::ConvertPointToScreen(target, &point_in_unified_host);
  D_OUT(marker, "In screen ...");
  D_OUT_VAL(marker, point_in_unified_host.ToString());
  // The display bounds of the mirroring windows isn't scaled, so
  // transform back to the host coordinates.
  target->GetHost()->GetRootTransform().TransformPoint(&point_in_unified_host);

  D_OUT(marker, "In Host ...");
  D_OUT_VAL(marker, point_in_unified_host.ToString());
  D_OUT_VAL(marker, current_cursor_display_id_);

  if (current_cursor_display_id_ != display::kInvalidDisplayId) {
    D_HERE(marker, "1");
    aura::client::CursorClient* cursor_client =
        aura::client::GetCursorClient(target->GetRootWindow());
    if (cursor_client) {
      display::Displays mirroring_display_list =
          Shell::Get()->display_manager()->software_mirroring_display_list();
      auto iter = display::FindDisplayContainingPoint(mirroring_display_list,
                                                      point_in_unified_host);
      if (iter != mirroring_display_list.end() &&
          current_cursor_display_id_ != iter->id()) {
        D_HERE(marker, "2");
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
  D_OUT_VAL(marker, point_in_native.ToString());

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
  D_OUT(marker, "After offset");
  D_OUT_VAL(marker, point_in_native.ToString());

  return WarpMouseCursorInNativeCoords(point_in_native, point_in_unified_host,
                                       update_location_for_test_);
}

void UnifiedMouseWarpController::SetEnabled(bool enabled) {
  // Mouse warp shuld be always on in Unified mode.
}

void UnifiedMouseWarpController::ComputeBounds() {
  auto marker = MARK_FUNC();
  D_OUT(marker, "/////////////////////////////////////////////////");
  D_OUT(marker, "/////////////////////////////////////////////////");
  display::Displays display_list =
      Shell::Get()->display_manager()->software_mirroring_display_list();

  if (display_list.size() < 2) {
    LOG(ERROR) << "Mirroring Display lost during re-configuration";
    return;
  }
  LOG_IF(ERROR, display_list.size() > 2) << "Only two displays are supported";

//  const gfx::Size unified_mat_dimens =
//      Shell::Get()->display_manager()->GetUnifiedModeDimensions();
//  const int rows = unified_mat_dimens.width();
//  const int columns = unified_mat_dimens.height();
  // FIXME: This won't work.
  for (size_t i = 0; i < display_list.size() - 1; ++i) {
    const display::Display& first = display_list[i];
    for (size_t j = i + 1; j < display_list.size(); ++j) {
      const display::Display& second = display_list[j];
      gfx::Rect first_edge;
      gfx::Rect second_edge;
      if (display::ComputeBoundary(first, second, &first_edge, &second_edge)) {
        first_edge =
            GetNativeEdgeBounds(GetMirroringAshWindowTreeHostForDisplayId(
                first.id()), first_edge);
        D_OUT_VAL(marker, first_edge.ToString());

        second_edge = GetNativeEdgeBounds(
            GetMirroringAshWindowTreeHostForDisplayId(second.id()),
                                                      second_edge);
        D_OUT_VAL(marker, second_edge.ToString());
        displays_edge_bounds_in_native_[first.id()].emplace_back(first_edge);
        displays_edge_bounds_in_native_[second.id()].emplace_back(second_edge);
      }
    }
  }











//  const display::Display& first = display_list[0];
//  const display::Display& second = display_list[1];
//
//  D_OUT_VAL(marker, first.bounds().ToString());
//  D_OUT_VAL(marker, second.bounds().ToString());
//
//  bool success =
//      display::ComputeBoundary(first, second, &first_edge_bounds_in_native_,
//                               &second_edge_bounds_in_native_);
//  D_OUT_VAL(marker, success);
//  D_OUT_VAL(marker, first_edge_bounds_in_native_.ToString());
//  D_OUT_VAL(marker, second_edge_bounds_in_native_.ToString());
//
//  DCHECK(success);
//
//  first_edge_bounds_in_native_ =
//      GetNativeEdgeBounds(GetMirroringAshWindowTreeHostForDisplayId(first.id()),
//                          first_edge_bounds_in_native_);
//
//  second_edge_bounds_in_native_ = GetNativeEdgeBounds(
//      GetMirroringAshWindowTreeHostForDisplayId(second.id()),
//      second_edge_bounds_in_native_);
//
//  D_OUT(marker, "After GetNativeEdgeBounds() ....");
//  D_OUT_VAL(marker, first_edge_bounds_in_native_.ToString());
//  D_OUT_VAL(marker, second_edge_bounds_in_native_.ToString());
}

bool UnifiedMouseWarpController::WarpMouseCursorInNativeCoords(
    const gfx::Point& point_in_native,
    const gfx::Point& point_in_unified_host,
    bool update_mouse_location_now) {
  auto marker = MARK_FUNC();
  D_OUT(marker, "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
  D_OUT(marker, "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");

  D_OUT_VAL(marker, first_edge_bounds_in_native_.ToString());
  D_OUT_VAL(marker, second_edge_bounds_in_native_.ToString());

  bool in_first_edge = first_edge_bounds_in_native_.Contains(point_in_native);
  bool in_second_edge = second_edge_bounds_in_native_.Contains(point_in_native);

  D_OUT_VAL(marker, in_first_edge);
  D_OUT_VAL(marker, in_second_edge);

  if (!in_first_edge && !in_second_edge) {
    D_OUT(marker, "Returning ....");
    return false;
  }

  display::Displays display_list =
      Shell::Get()->display_manager()->software_mirroring_display_list();
  // Wait updating the cursor until the cursor moves to the new display
  // to avoid showing the wrong sized cursor at the source display.
  current_cursor_display_id_ =
      in_first_edge ? display_list[0].id() : display_list[1].id();

  D_OUT_VAL(marker, current_cursor_display_id_);

  AshWindowTreeHost* target_ash_host =
      GetMirroringAshWindowTreeHostForDisplayId(
          in_first_edge ? display_list[1].id() : display_list[0].id());
  MoveCursorTo(target_ash_host, point_in_unified_host,
               update_mouse_location_now);
  return true;
}

}  // namespace ash
