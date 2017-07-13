// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_controller.h"

#include <cmath>

#include "ash/ash_switches.h"
#include "ash/highlighter/highlighter_delegate.h"
#include "ash/highlighter/highlighter_result_view.h"
#include "ash/highlighter/highlighter_view.h"
#include "ash/shell.h"
#include "ash/system/palette/palette_utils.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

const float kHorizontalStrokeLengthThreshold = 20;
const float kHorizontalStrokeThicknessThreshold = 2;
const float kHorizontalStrokeFlatnessThreshold = 0.1;

bool DetectHorizontalStroke(const gfx::Rect& box,
                            const gfx::Size& pen_tip_size,
                            const std::vector<gfx::PointF> points) {
  return box.width() > kHorizontalStrokeLengthThreshold &&
         box.height() <
             pen_tip_size.height() * kHorizontalStrokeThicknessThreshold &&
         box.height() < box.width() * kHorizontalStrokeFlatnessThreshold;
}

gfx::Rect AdjustHorizontalStroke(const gfx::Rect& box,
                                 const gfx::Size& pen_tip_size) {
  return gfx::Rect(box.x() - pen_tip_size.width() / 2,
                   box.CenterPoint().y() - pen_tip_size.height() / 2,
                   box.width() + pen_tip_size.width(), pen_tip_size.height());
}

const double kClosedShapeWrapThreshold = M_PI * 2 * 0.95;
const double kClosedShapeSweepThreshold = M_PI * 2 * 0.8;
const double kClosedShapeJiggleThreshold = 0.1;

bool DetectClosedShape(const gfx::Rect& box,
                       const std::vector<gfx::PointF> points) {
  if (points.size() < 3) {
    return false;
  }

  const gfx::Point center = box.CenterPoint();

  // Analyze vectors pointing from the center to each point.
  // Compute the cumulative swept angle and count positive
  // and negative angles separately.
  double sweptAngle = 0;
  int positive = 0;
  int negative = 0;

  double prevAngle = 0;
  bool hasPrevAngle = false;

  for (gfx::PointF p : points) {
    const double angle = atan2(p.y() - center.y(), p.x() - center.x());
    if (hasPrevAngle) {
      double diffAngle = angle - prevAngle;
      if (diffAngle > kClosedShapeWrapThreshold) {
        diffAngle -= M_PI * 2;
      } else if (diffAngle < -kClosedShapeWrapThreshold) {
        diffAngle += M_PI * 2;
      }
      sweptAngle += diffAngle;
      if (diffAngle > 0) {
        positive++;
      }
      if (diffAngle < 0) {
        negative++;
      }
    } else {
      hasPrevAngle = true;
    }
    prevAngle = angle;
  }

  if (std::abs(sweptAngle) < kClosedShapeSweepThreshold) {
    // Has not swept enough of the full circle.
    return false;
  }

  if (sweptAngle > 0 &&
      ((double)negative / positive) > kClosedShapeJiggleThreshold) {
    // Main direction is positive, but went too often in the negative direction.
    return false;
  }
  if (sweptAngle < 0 &&
      ((double)positive / negative) > kClosedShapeJiggleThreshold) {
    // Main direction is negative, but went too often in the positive direction.
    return false;
  }

  return true;
}

}  // namespace

HighlighterController::HighlighterController() {
  Shell::Get()->AddPreTargetHandler(this);
}

HighlighterController::~HighlighterController() {
  Shell::Get()->RemovePreTargetHandler(this);
}

void HighlighterController::EnableHighlighter(
    HighlighterDelegate* highlighter_delegate) {
  highlighter_delegate_ = highlighter_delegate;
}

void HighlighterController::DisableHighlighter() {
  highlighter_delegate_ = nullptr;
}

void HighlighterController::OnTouchEvent(ui::TouchEvent* event) {
  if (!highlighter_delegate_)
    return;

  if (event->pointer_details().pointer_type !=
      ui::EventPointerType::POINTER_TYPE_PEN)
    return;

  if (event->type() != ui::ET_TOUCH_MOVED &&
      event->type() != ui::ET_TOUCH_PRESSED &&
      event->type() != ui::ET_TOUCH_RELEASED)
    return;

  // Find the root window that the event was captured on. We never need to
  // switch between different root windows because it is not physically possible
  // to seamlessly drag a finger between two displays like it is with a mouse.
  gfx::Point event_location = event->root_location();
  aura::Window* current_window =
      static_cast<aura::Window*>(event->target())->GetRootWindow();

  // Start a new highlighter session if the stylus is pressed but not
  // pressed over the palette.
  if (event->type() == ui::ET_TOUCH_PRESSED &&
      !palette_utils::PaletteContainsPointInScreen(event_location)) {
    DestroyHighlighterView();
    UpdateHighlighterView(current_window, event->root_location_f(), event);
  }

  if (event->type() == ui::ET_TOUCH_MOVED && highlighter_view_) {
    UpdateHighlighterView(current_window, event->root_location_f(), event);
  }

  if (event->type() == ui::ET_TOUCH_RELEASED && highlighter_view_) {
    const gfx::Rect box = highlighter_view_->GetBoundingBox();

    const float scale_factor = current_window->layer()->device_scale_factor();

    const gfx::Size pen_tip_size = highlighter_view_->GetPenTipSize();
    if (DetectHorizontalStroke(box, pen_tip_size,
                               highlighter_view_->points())) {
      const gfx::Rect adjusted_box = AdjustHorizontalStroke(box, pen_tip_size);
      highlighter_delegate_->HandleSelection(
          gfx::ScaleToEnclosingRect(adjusted_box, scale_factor));
      highlighter_view_->AnimateInPlace();

      result_view_.reset(new HighlighterResultView(current_window));
      result_view_->AnimateInPlace(adjusted_box,
                                   highlighter_view_->GetPenColor());
    } else if (DetectClosedShape(box, highlighter_view_->points())) {
      highlighter_delegate_->HandleSelection(
          gfx::ScaleToEnclosingRect(box, scale_factor));
      highlighter_view_->AnimateInflate();

      result_view_.reset(new HighlighterResultView(current_window));
      result_view_->AnimateDeflate(box);
    } else {
      highlighter_view_->AnimateDeflate();
    }
  }
}

void HighlighterController::OnWindowDestroying(aura::Window* window) {
  SwitchTargetRootWindowIfNeeded(window);
}

void HighlighterController::SwitchTargetRootWindowIfNeeded(
    aura::Window* root_window) {
  if (!root_window) {
    DestroyHighlighterView();
  }

  if (!highlighter_view_ && highlighter_delegate_) {
    highlighter_view_.reset(new HighlighterView(root_window));
  }
}

void HighlighterController::UpdateHighlighterView(
    aura::Window* current_window,
    const gfx::PointF& event_location,
    ui::Event* event) {
  SwitchTargetRootWindowIfNeeded(current_window);
  highlighter_view_->AddNewPoint(event_location);
  event->StopPropagation();
}

void HighlighterController::DestroyHighlighterView() {
  highlighter_view_.reset();
  result_view_.reset();
}

}  // namespace ash
