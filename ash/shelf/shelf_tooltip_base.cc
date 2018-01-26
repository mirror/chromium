// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_tooltip_base.h"

#include "ash/shelf/shelf.h"
#include "ui/events/event.h"

namespace ash {

ShelfTooltipBase::ShelfTooltipBase(Shelf* shelf) : shelf_(shelf) {}

ShelfTooltipBase::~ShelfTooltipBase() = default;

views::BubbleBorder::Arrow ShelfTooltipBase::GetBubbleArrow() {
  views::BubbleBorder::Arrow arrow = views::BubbleBorder::Arrow::NONE;

  switch (shelf_->alignment()) {
    case SHELF_ALIGNMENT_BOTTOM:
    case SHELF_ALIGNMENT_BOTTOM_LOCKED:
      arrow = views::BubbleBorder::BOTTOM_CENTER;
      break;
    case SHELF_ALIGNMENT_LEFT:
      arrow = views::BubbleBorder::LEFT_CENTER;
      break;
    case SHELF_ALIGNMENT_RIGHT:
      arrow = views::BubbleBorder::RIGHT_CENTER;
      break;
  }
  return arrow;
}

void ShelfTooltipBase::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  // Close on any press events inside or outside the tooltip.
  if (event.type() == ui::ET_POINTER_DOWN)
    Close();
}

void ShelfTooltipBase::WillChangeVisibilityState(
    ShelfVisibilityState new_state) {
  if (new_state == SHELF_HIDDEN)
    Close();
}

}  // namespace ash
