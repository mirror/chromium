// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/screen_util.h"

#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "base/logging.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

// static
gfx::Rect ScreenUtil::GetMaximizedWindowBoundsInParent(aura::Window* window) {
  if (Shelf::ForWindow(window)->shelf_widget())
    return GetDisplayWorkAreaBoundsInParent(window);
  return GetDisplayBoundsInParent(window);
}

// static
gfx::Rect ScreenUtil::GetDisplayBoundsInParent(aura::Window* window) {
  gfx::Rect result =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window).bounds();
  ::wm::ConvertRectFromScreen(window->parent(), &result);
  return result;
}

// static
gfx::Rect ScreenUtil::GetDisplayWorkAreaBoundsInParent(aura::Window* window) {
  gfx::Rect result =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window).work_area();
  ::wm::ConvertRectFromScreen(window->parent(), &result);
  return result;
}

// static
gfx::Rect ScreenUtil::GetDisplayWorkAreaBoundsInParentForLockScreen(
    aura::Window* window) {
  gfx::Rect bounds = Shelf::ForWindow(window)->GetUserWorkAreaBounds();
  ::wm::ConvertRectFromScreen(window->parent(), &bounds);
  return bounds;
}

// static
gfx::Rect ScreenUtil::GetDisplayBoundsWithShelf(aura::Window* window) {
  if (Shell::Get()->display_manager()->IsInUnifiedMode()) {
    // In unified desktop mode, there is only one shelf in the primary mirroing
    // display.
    const display::DisplayManager* display_manager =
        Shell::Get()->display_manager();
    const display::Display* first_display =
        display_manager->GetPrimaryMirroringDisplayForUnifiedDesktop();
    DCHECK(first_display);

    // TODO(afakhry): Simplify this somehow.
    const auto& unified_display_info = display_manager->GetDisplayInfo(
        display::Screen::GetScreen()->GetPrimaryDisplay().id());
    const int unified_logical_height =
        window->GetRootWindow()->bounds().height();
    const int unified_physical_height =
        unified_display_info.bounds_in_native().height();
    const float unified_height_scale =
        unified_logical_height / static_cast<float>(unified_physical_height);

    const int row_index =
        display_manager->GetMirroringDisplayRowIndexInUnifiedMatrix(
            first_display->id());
    const int row_physical_height = display_manager->GetRowMaxHeight(row_index);
    const int row_logical_height = row_physical_height * unified_height_scale;

    gfx::SizeF size(first_display->size());
    const float scale = row_logical_height / size.height();
    size.Scale(scale, scale);
    return gfx::Rect(gfx::ToCeiledSize(size));
  }

  return window->GetRootWindow()->bounds();
}

}  // namespace ash
