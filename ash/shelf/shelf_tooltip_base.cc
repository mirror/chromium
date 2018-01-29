// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_tooltip_base.h"

#include "ash/shelf/shelf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace ash {
namespace {
const int kTooltipAppearanceDelay = 1000;  // msec
}  // namespace

ShelfTooltipBase::ShelfTooltipBase(Shelf* shelf)
    : timer_delay_(kTooltipAppearanceDelay),
      shelf_(shelf),
      weak_factory_(this) {}

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

bool ShelfTooltipBase::IsVisible() const {
  return bubble_ && bubble_->GetWidget()->IsVisible();
}

void ShelfTooltipBase::Close() {
  timer_.Stop();
  if (bubble_)
    bubble_->GetWidget()->Close();
  bubble_ = nullptr;
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

void ShelfTooltipBase::OnAutoHideStateChanged(ShelfAutoHideState new_state) {
  if (new_state == SHELF_AUTO_HIDE_HIDDEN) {
    timer_.Stop();
    // AutoHide state change happens during an event filter, so immediate close
    // may cause a crash in the HandleMouseEvent() after the filter.  So we just
    // schedule the Close here.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&ShelfTooltipBase::Close, weak_factory_.GetWeakPtr()));
  }
}

}  // namespace ash
