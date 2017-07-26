// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_controller.h"

#include "ash/highlighter/highlighter_gesture_util.h"
#include "ash/highlighter/highlighter_result_view.h"
#include "ash/highlighter/highlighter_selection_observer.h"
#include "ash/highlighter/highlighter_view.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Adjust the height of the bounding box to match the pen tip height,
// while keeping the same vertical center line. Adjust the width to
// account for the pen tip width.
gfx::RectF AdjustHorizontalStroke(const gfx::Rect& box,
                                  const gfx::SizeF& pen_tip_size) {
  return gfx::RectF(box.x() - pen_tip_size.width() / 2,
                    box.CenterPoint().y() - pen_tip_size.height() / 2,
                    box.width() + pen_tip_size.width(), pen_tip_size.height());
}

}  // namespace

HighlighterController::HighlighterController() : observer_(nullptr) {}

HighlighterController::~HighlighterController() {}

void HighlighterController::SetObserver(
    HighlighterSelectionObserver* observer) {
  observer_ = observer;
}

views::View* HighlighterController::GetPointerView() const {
  return highlighter_view_.get();
}

void HighlighterController::CreatePointerView(
    base::TimeDelta presentation_delay,
    aura::Window* root_window) {
  highlighter_view_ =
      base::MakeUnique<HighlighterView>(presentation_delay, root_window);
  result_view_.reset();
}

void HighlighterController::UpdatePointerView(ui::TouchEvent* event) {
  highlighter_view_->AddNewPoint(event->root_location_f(), event->time_stamp());

  if (event->type() == ui::ET_TOUCH_RELEASED && observer_) {
    aura::Window* root_window =
        highlighter_view_->GetWidget()->GetNativeWindow()->GetRootWindow();
    const gfx::Rect box = highlighter_view_->GetBoundingBox();
    const gfx::Point pivot(box.CenterPoint());

    const float scale_factor = root_window->layer()->device_scale_factor();

    HighlighterView::AnimationMode animation_mode;

    if (DetectHorizontalStroke(box, HighlighterView::kPenTipSize)) {
      animation_mode = HighlighterView::AnimationMode::kFadeout;
      const gfx::Rect box_adjusted = gfx::ToEnclosingRect(
          AdjustHorizontalStroke(box, HighlighterView::kPenTipSize));
      observer_->HandleSelection(
          gfx::ScaleToEnclosingRect(box_adjusted, scale_factor));
      result_view_ = base::MakeUnique<HighlighterResultView>(root_window);
      result_view_->AnimateInPlace(
          box_adjusted, HighlighterView::kPenColor,
          base::Bind(&HighlighterController::DestroyResultView,
                     base::Unretained(this)));
    } else if (DetectClosedShape(box, highlighter_view_->GetTrace())) {
      animation_mode = HighlighterView::AnimationMode::kInflate;
      observer_->HandleSelection(gfx::ScaleToEnclosingRect(box, scale_factor));
      result_view_ = base::MakeUnique<HighlighterResultView>(root_window);
      result_view_->AnimateDeflate(
          box, base::Bind(&HighlighterController::DestroyResultView,
                          base::Unretained(this)));
    } else {
      animation_mode = HighlighterView::AnimationMode::kDeflate;
    }

    highlighter_view_->Animate(
        pivot, animation_mode,
        base::Bind(&HighlighterController::DestroyHighlighterView,
                   base::Unretained(this)));
  }
}

void HighlighterController::DestroyPointerView() {
  DestroyHighlighterView();
  DestroyResultView();
}

void HighlighterController::DestroyHighlighterView() {
  highlighter_view_.reset();
}

void HighlighterController::DestroyResultView() {
  result_view_.reset();
}

}  // namespace ash
