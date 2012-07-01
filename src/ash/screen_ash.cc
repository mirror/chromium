// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/screen_ash.h"

#include "ash/shell.h"
#include "ash/wm/shelf_layout_manager.h"
#include "base/logging.h"
#include "ui/aura/env.h"
#include "ui/aura/display_manager.h"
#include "ui/aura/root_window.h"
#include "ui/gfx/display.h"
#include "ui/gfx/screen.h"

namespace ash {

namespace {
aura::DisplayManager* GetDisplayManager() {
  return aura::Env::GetInstance()->display_manager();
}
}  // namespace

ScreenAsh::ScreenAsh() {
}

ScreenAsh::~ScreenAsh() {
}

// static
gfx::Rect ScreenAsh::GetMaximizedWindowBounds(aura::Window* window) {
  if (window->GetRootWindow() == Shell::GetPrimaryRootWindow())
    return Shell::GetInstance()->shelf()->GetMaximizedWindowBounds(window);
  else
    return gfx::Screen::GetDisplayNearestWindow(window).bounds();
}

// static
gfx::Rect ScreenAsh::GetUnmaximizedWorkAreaBounds(aura::Window* window) {
  if (window->GetRootWindow() == Shell::GetPrimaryRootWindow())
    return Shell::GetInstance()->shelf()->GetUnmaximizedWorkAreaBounds(window);
  else
    return gfx::Screen::GetDisplayNearestWindow(window).work_area();
}

gfx::Point ScreenAsh::GetCursorScreenPoint() {
  // TODO(oshima): Support multiple root window.
  return Shell::GetPrimaryRootWindow()->last_mouse_location();
}

gfx::NativeWindow ScreenAsh::GetWindowAtCursorScreenPoint() {
  const gfx::Point point = gfx::Screen::GetCursorScreenPoint();
  // TODO(oshima): convert point to relateive to the root window.
  return Shell::GetRootWindowAt(point)->GetTopWindowContainingPoint(point);
}

int ScreenAsh::GetNumDisplays() {
  return GetDisplayManager()->GetNumDisplays();
}

gfx::Display ScreenAsh::GetDisplayNearestWindow(gfx::NativeView window) const {
  return GetDisplayManager()->GetDisplayNearestWindow(window);
}

gfx::Display ScreenAsh::GetDisplayNearestPoint(const gfx::Point& point) const {
  return GetDisplayManager()->GetDisplayNearestPoint(point);
}

gfx::Display ScreenAsh::GetPrimaryDisplay() const {
  return *GetDisplayManager()->GetDisplayAt(0);
}

}  // namespace ash
