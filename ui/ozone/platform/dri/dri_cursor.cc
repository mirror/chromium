// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/dri/dri_cursor.h"

#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/ozone/platform/dri/dri_gpu_platform_support_host.h"
#include "ui/ozone/platform/dri/dri_window.h"
#include "ui/ozone/platform/dri/dri_window_manager.h"

#if defined(OS_CHROMEOS)
#include "ui/events/ozone/chromeos/cursor_controller.h"
#endif

namespace ui {

DriCursor::DriCursor(DriWindowManager* window_manager,
                     DriGpuPlatformSupportHost* sender)
    : window_manager_(window_manager),
      sender_(sender),
      cursor_window_(gfx::kNullAcceleratedWidget) {
}

DriCursor::~DriCursor() {
}

void DriCursor::SetCursor(gfx::AcceleratedWidget widget,
                          PlatformCursor platform_cursor) {
  DCHECK_NE(widget, gfx::kNullAcceleratedWidget);
  scoped_refptr<BitmapCursorOzone> cursor =
      BitmapCursorFactoryOzone::GetBitmapCursor(platform_cursor);
  if (cursor_ == cursor || cursor_window_ != widget)
    return;

  cursor_ = cursor;

  ShowCursor();
}

void DriCursor::ShowCursor() {
  DCHECK_NE(cursor_window_, gfx::kNullAcceleratedWidget);
  if (cursor_.get())
    sender_->SetHardwareCursor(cursor_window_, cursor_->bitmaps(),
                               bitmap_location(), cursor_->frame_delay_ms());
  else
    HideCursor();
}

void DriCursor::HideCursor() {
  DCHECK_NE(cursor_window_, gfx::kNullAcceleratedWidget);
  sender_->SetHardwareCursor(cursor_window_, std::vector<SkBitmap>(),
                             gfx::Point(), 0);
}

void DriCursor::ConfineCursorToBounds(const gfx::Rect& bounds) {
  cursor_confined_bounds_ = bounds;
  MoveCursorTo(cursor_location_);
  ShowCursor();
}

void DriCursor::MoveCursorTo(gfx::AcceleratedWidget widget,
                             const gfx::PointF& location) {
  // When moving between windows hide the cursor on the current window then show
  // it on the new window.
  bool changing_window =
      widget != cursor_window_ && cursor_window_ != gfx::kNullAcceleratedWidget;

  if (changing_window)
    HideCursor();

  DriWindow* window = window_manager_->GetWindow(widget);

  cursor_window_ = widget;
  cursor_location_ = location;
  cursor_display_bounds_ = window->GetBounds();
  cursor_confined_bounds_ = window->GetCursorConfinedBounds();

  cursor_location_.SetToMax(cursor_confined_bounds_.origin());
  // Right and bottom edges are exclusive.
  cursor_location_.SetToMin(gfx::PointF(cursor_confined_bounds_.right() - 1,
                                        cursor_confined_bounds_.bottom() - 1));

  if (cursor_.get()) {
    if (changing_window)
      ShowCursor();
    else
      sender_->MoveHardwareCursor(cursor_window_, bitmap_location());
  }
}

void DriCursor::MoveCursorTo(const gfx::PointF& location) {
  DriWindow* window =
      window_manager_->GetWindowAt(gfx::ToFlooredPoint(location));
  if (!window)
    return;

  MoveCursorTo(window->GetAcceleratedWidget(),
               location - window->GetBounds().OffsetFromOrigin());
}

void DriCursor::MoveCursor(const gfx::Vector2dF& delta) {
  if (cursor_window_ == gfx::kNullAcceleratedWidget)
    return;

#if defined(OS_CHROMEOS)
  gfx::Vector2dF transformed_delta = delta;
  ui::CursorController::GetInstance()->ApplyCursorConfigForWindow(
      cursor_window_, &transformed_delta);
  MoveCursorTo(cursor_window_, cursor_location_ + transformed_delta);
#else
  MoveCursorTo(cursor_window_, cursor_location_ + delta);
#endif
}

gfx::Rect DriCursor::GetCursorConfinedBounds() {
  return cursor_confined_bounds_ +
      cursor_display_bounds_.OffsetFromOrigin();
}

gfx::AcceleratedWidget DriCursor::GetCursorWindow() {
  return cursor_window_;
}

void DriCursor::Reset() {
  cursor_window_ = gfx::kNullAcceleratedWidget;
  cursor_ = nullptr;
}

bool DriCursor::IsCursorVisible() {
  return cursor_.get();
}

gfx::PointF DriCursor::GetLocation() {
  return cursor_location_ + cursor_display_bounds_.OffsetFromOrigin();
}

gfx::Point DriCursor::bitmap_location() {
  return gfx::ToFlooredPoint(cursor_location_) -
         cursor_->hotspot().OffsetFromOrigin();
}

}  // namespace ui
