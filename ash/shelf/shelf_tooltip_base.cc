// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_tooltip_base.h"

#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_tooltip_bubble.h"
#include "ash/shelf/shelf_widget.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/wm/core/window_animations.h"

namespace ash {
namespace {
// The time delay for showing the first tooltip, measured in msec.
constexpr int kTooltipAppearanceDelay = 1000;
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

void ShelfTooltipBase::ShowTooltipWithDelay(views::View* view) {
  if (ShouldShowTooltipForView(view)) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(timer_delay_),
                 base::Bind(&ShelfTooltipBase::ShowTooltip,
                            weak_factory_.GetWeakPtr(), view));
  }
}

void ShelfTooltipBase::ShowTooltipWithText(views::View* view,
                                           const base::string16& text,
                                           bool asymmetrical_border) {
  timer_.Stop();
  if (bubble_) {
    // Cancel the hiding animation to hide the old bubble immediately.
    ::wm::SetWindowVisibilityAnimationTransition(
        bubble_->GetWidget()->GetNativeWindow(), ::wm::ANIMATE_NONE);
    Close();
  }

  if (!ShouldShowTooltipForView(view))
    return;

  bubble_ = new ShelfTooltipBubble(view, GetBubbleArrow(), text,
                                   shelf_->shelf_widget()->GetNativeTheme(),
                                   asymmetrical_border);
  aura::Window* window = bubble_->GetWidget()->GetNativeWindow();
  ::wm::SetWindowVisibilityAnimationType(
      window, ::wm::WINDOW_VISIBILITY_ANIMATION_TYPE_VERTICAL);
  ::wm::SetWindowVisibilityAnimationTransition(window, ::wm::ANIMATE_HIDE);
  bubble_->GetWidget()->Show();
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
