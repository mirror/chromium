// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "chrome/browser/ui/views/omnibox/omnibox_keyword_search_button.h"

#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
// FIXME: I don't like this next include. See similar FIXME in
//        omnibox_result_view.h.
#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/events/event.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/controls/button/button.h"

// FIXME: put this in the result view instead?
void OmniboxKeywordSearchButton::ButtonPressed(views::Button* sender,
                                               const ui::Event& event) {
  LOG(ERROR) << "ButtonPressed";
  /*
  // Ignore mouse events that aren't left or middle clicks.
  if (event.IsMouseEvent()) {
    const ui::MouseEvent* mouse = event.AsMouseEvent();
    if (!(mouse->IsOnlyLeftMouseButton() || mouse->IsOnlyMiddleMouseButton()))
      return;
  }

  // FIXME: Exit early if we're in keyword state (on the left)

  ClearState();

  LOG(ERROR) << "HERE WE GO!";
  // FIXME: pass touch vs. click parameter here. Also, should there be new
  // touch/click params for this being a different event source?
  result_view_->AcceptKeyword();
  */
}

/*
bool OmniboxKeywordSearchButton::OnMouseDragged(const ui::MouseEvent& event) {
  LOG(ERROR) << "OnMouseDragged";
  if (HitTestPoint(event.location())) {
    return true;
  } else {
    SetBackground(nullptr);
    SchedulePaint();
    //SetMouseHandler(result_view_->model());
    return true;
  }
}
*/

// FIXME: how to make this work for touch UI?
bool OmniboxKeywordSearchButton::OnMousePressed(const ui::MouseEvent& event) {
  /*
  LOG(ERROR) << "OnMousePressed";
  result_view_->model()->SetSelectedLine(result_view_->model_index());

  SetPressed();

  return ImageButton::OnMousePressed(event);
  */
  return false;
}

void OmniboxKeywordSearchButton::SetPressed() {
  const SkColor bg_color =
      color_utils::AlphaBlend(SK_ColorBLACK, SK_ColorTRANSPARENT, 0.4 * 255);
  SetBackground(base::MakeUnique<BackgroundWith1PxBorder>(bg_color, bg_color));
  SchedulePaint();
}

void OmniboxKeywordSearchButton::ClearState() {
  SetBackground(nullptr);
  SchedulePaint();
}

// FIXME: introduce a SetHovered method
void OmniboxKeywordSearchButton::OnMouseMoved(const ui::MouseEvent& event) {
  // LOG(ERROR) << "OnMouseMoved";
  /*
  const SkColor bg_color =
      GetNativeTheme()->GetSystemColor(
          NativeTheme::kColorId_ResultsTableSelectedBackground);
      //GetColor(OmniboxResultView::SELECTED, OmniboxResultView::BACKGROUND);
  */
  /*
  result_view_->SetHovered(true);

  // FIXME: Move this above SetHovered and skip the SchedulePaint?
  const SkColor bg_color =
      color_utils::AlphaBlend(SK_ColorBLACK, SK_ColorTRANSPARENT, 0.2 * 255);
  SetBackground(base::MakeUnique<BackgroundWith1PxBorder>(bg_color, bg_color));
  SchedulePaint();
  */

  if (!result_view_->AnimationRunning()) {
    const SkColor bg_color =
        color_utils::AlphaBlend(SK_ColorBLACK, SK_ColorTRANSPARENT, 0.2 * 255);
    // FIXME: Introduce 1px background w/ half rounded corners (based on which
    //        side it's on).
    SetBackground(
        base::MakeUnique<BackgroundWith1PxBorder>(bg_color, bg_color));
    SchedulePaint();
  } else {
    ClearState();
  }

  result_view_->SetHovered(true);
}

void OmniboxKeywordSearchButton::OnMouseExited(const ui::MouseEvent& event) {
  LOG(ERROR) << "OnMouseExited";
  result_view_->SetHovered(false);

  ClearState();
}

// Do this instead of setting a custom background class?
/*
// views::View:
void OnPaint(gfx::Canvas* canvas) override {
  views::View::OnPaint(canvas);
  SkScalar radius = SkIntToScalar(corner_radius_);
  const SkScalar kRadius[8] = {radius, radius, radius, radius, 0, 0, 0, 0};
  SkPath path;
  gfx::Rect bounds(size());
  path.addRoundRect(gfx::RectToSkRect(bounds), kRadius);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  canvas->ClipPath(path, true);

  SkColor target_color = initial_color_;
  if (target_color_ != target_color) {
    target_color = color_utils::AlphaBlend(target_color_, initial_color_,
                                           current_value_);
  }
  canvas->DrawColor(target_color);
}
*/
