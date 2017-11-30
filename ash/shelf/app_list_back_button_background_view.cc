// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/app_list_back_button_background_view.h"

#include "ash/shelf/app_list_button.h"
#include "ash/shelf/back_button.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/canvas.h"

namespace ash {

namespace {

bool IsTabletMode() {
  return Shell::Get()->tablet_mode_controller() &&
         Shell::Get()
             ->tablet_mode_controller()
             ->IsTabletModeWindowManagerEnabled();
}

}  // namespace

const char AppListBackButtonBackgroundView::kViewClassName[] =
    "AppListBackButtonBackgroundView";

AppListBackButtonBackgroundView::AppListBackButtonBackgroundView(
    InkDropButtonListener* listener,
    ShelfView* shelf_view,
    Shelf* shelf)
    : background_color_(kShelfDefaultBaseColor), shelf_(shelf) {
  app_list_button_ = new AppListButton(listener, shelf_view, shelf);
  back_button_ = new BackButton(shelf_view, shelf);

  AddChildView(back_button_);
  AddChildView(app_list_button_);
}

AppListBackButtonBackgroundView::~AppListBackButtonBackgroundView() = default;

void AppListBackButtonBackgroundView::UpdateShelfItemBackground(SkColor color) {
  background_color_ = color;
  SchedulePaint();
}

void AppListBackButtonBackgroundView::OnBoundsAnimationStarted() {}

void AppListBackButtonBackgroundView::OnBoundsAnimationFinished() {
  Layout();
  SchedulePaint();
}

const char* AppListBackButtonBackgroundView::GetClassName() const {
  return kViewClassName;
}

void AppListBackButtonBackgroundView::Layout() {
  gfx::Point app_list_button_origin;
  if (shelf_->is_tablet_mode_animation_running() || IsTabletMode())
    app_list_button_origin = gfx::Point(width() - kShelfButtonSize, 0);
  app_list_button_->SetBoundsRect(gfx::Rect(
      app_list_button_origin, gfx::Size(kShelfButtonSize, kShelfButtonSize)));

  back_button_->SetBounds(0, 0, kShelfButtonSize, kShelfButtonSize);
  back_button_->SetVisible(IsTabletMode() ||
                           shelf_->is_tablet_mode_animation_running());
}

void AppListBackButtonBackgroundView::OnPaint(gfx::Canvas* canvas) {
  gfx::PointF back_center(back_button_->bounds().CenterPoint());
  gfx::PointF circle_center(app_list_button_->bounds().CenterPoint());

  float min_x = std::min(back_center.x(), circle_center.x());

  gfx::RectF background_bounds(
      shelf_->PrimaryAxisValue(min_x, min_x - kAppListButtonRadius),
      shelf_->PrimaryAxisValue(back_center.y() - kAppListButtonRadius,
                               back_center.y()),
      shelf_->PrimaryAxisValue(std::abs(circle_center.x() - back_center.x()),
                               2 * kAppListButtonRadius),
      shelf_->PrimaryAxisValue(2 * kAppListButtonRadius,
                               std::abs(circle_center.y() - back_center.y())));

  // Create the path by drawing two circles, one around the back button and
  // one around the app list circle. Join them with the rectangle calculated
  // previously.
  SkPath path;
  path.addCircle(circle_center.x(), circle_center.y(), kAppListButtonRadius);
  path.addCircle(back_center.x(), back_center.y(), kAppListButtonRadius);
  path.addRect(background_bounds.x(), background_bounds.y(),
               background_bounds.right(), background_bounds.bottom());

  cc::PaintFlags bg_flags;
  bg_flags.setColor(background_color_);
  bg_flags.setAntiAlias(true);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawPath(path, bg_flags);
}

}  // namespace ash
