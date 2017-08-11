// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/shaped_app_window_targeter.h"

#include "chrome/browser/ui/views/apps/chrome_native_app_window_views.h"
#include "ui/gfx/path.h"

ShapedAppWindowTargeter::ShapedAppWindowTargeter(
    ChromeNativeAppWindowViews* app_window)
    : app_window_(app_window) {}

ShapedAppWindowTargeter::~ShapedAppWindowTargeter() {}

bool ShapedAppWindowTargeter::GetHitTestRects(
    aura::Window* window,
    gfx::Rect* hit_test_rect_mouse,
    gfx::Rect* hit_test_rect_touch) const {
  return true;
}

std::unique_ptr<aura::WindowTargeter::ShapeRects>
ShapedAppWindowTargeter::GetHitTestShapeRects(aura::Window* target) const {
  auto shape_rects = base::MakeUnique<aura::WindowTargeter::ShapeRects>(
      *app_window_->shape_rects());
  return shape_rects;
}
