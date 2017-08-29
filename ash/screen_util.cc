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

#include "my_out.h"

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
    auto marker = MARK_FUNC();
    // In unified desktop mode, there is only one shelf in the first display.
    const display::Display& first_display =
        Shell::Get()->display_manager()->software_mirroring_display_list()[0];
    gfx::SizeF size(first_display.size());
    D_OUT_VAL(marker, size.ToString());
    D_OUT_VAL(marker, window->GetRootWindow()->bounds().height());
    float scale =
        window->GetRootWindow()->bounds().size().GetArea() /
        (size.GetArea() * Shell::Get()->display_manager()->GetUnifiedModeDimensions().GetArea());
    D_OUT_VAL(marker, scale);
    size.Scale(scale, scale);

    D_OUT_VAL(marker, size.ToString());
    return gfx::Rect(gfx::ToCeiledSize(size));
  }

  return window->GetRootWindow()->bounds();
}

}  // namespace ash
