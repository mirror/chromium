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
gfx::Rect AdjustHorizontalStroke(const gfx::Rect& box,
                                 const gfx::SizeF& pen_tip_size) {
  return gfx::ToEnclosingRect(
      gfx::RectF(box.x() - pen_tip_size.width() / 2,
                 box.CenterPoint().y() - pen_tip_size.height() / 2,
                 box.width() + pen_tip_size.width(), pen_tip_size.height()));
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

  if (event->type() == ui::ET_TOUCH_RELEASED) {
    aura::Window* root_window =
        highlighter_view_->GetWidget()->GetNativeWindow()->GetRootWindow();
    gfx::Rect box = highlighter_view_->GetBoundingBox();

    bool detected_stroke = false;
    HighlighterView::AnimationMode stroke_animation_mode =
        HighlighterView::AnimationMode::kDeflate;
    HighlighterResultView::AnimationMode result_animation_mode;

    if (DetectHorizontalStroke(box, HighlighterView::kPenTipSize)) {
      detected_stroke = true;
      stroke_animation_mode = HighlighterView::AnimationMode::kFadeout;
      result_animation_mode = HighlighterResultView::AnimationMode::kFadein;
      box = AdjustHorizontalStroke(box, HighlighterView::kPenTipSize);
    } else if (DetectClosedShape(box, highlighter_view_->GetTrace())) {
      detected_stroke = true;
      stroke_animation_mode = HighlighterView::AnimationMode::kInflate;
      result_animation_mode = HighlighterResultView::AnimationMode::kDeflate;
    }

    highlighter_view_->Animate(
        box.CenterPoint(), stroke_animation_mode,
        base::Bind(&HighlighterController::DestroyHighlighterView,
                   base::Unretained(this)));

    if (detected_stroke) {
      result_view_ = base::MakeUnique<HighlighterResultView>(root_window);
      result_view_->Animate(
          box, result_animation_mode,
          base::Bind(&HighlighterController::DestroyResultView,
                     base::Unretained(this)));

      if (observer_) {
        const float scale_factor = root_window->layer()->device_scale_factor();
        observer_->HandleSelection(
            gfx::ScaleToEnclosingRect(box, scale_factor));
      }
    }
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
