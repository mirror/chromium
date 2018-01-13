// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/status_area_tooltip_manager.h"

#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shelf/shelf_tooltip_bubble.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell_port.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/wm/window_util.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {
constexpr int kTooltipAppearanceDelay = 1000;  // msec
}  // namespace

StatusAreaTooltipManager::StatusAreaTooltipManager(
    StatusAreaWidgetDelegate* delegate,
    Shelf* shelf)
    : delegate_(delegate), shelf_(shelf), weak_factory_(this) {
  shelf_->AddObserver(this);
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);
}

StatusAreaTooltipManager::~StatusAreaTooltipManager() {
  ShellPort::Get()->RemovePointerWatcher(this);
  shelf_->RemoveObserver(this);
  aura::Window* window = nullptr;
  if (shelf_->GetStatusAreaWidget())
    window = shelf_->GetStatusAreaWidget()->GetNativeWindow();

  if (window)
    wm::RemoveLimitedPreTargetHandlerForWindow(this, window);
}

void StatusAreaTooltipManager::Init() {
  if (delegate_->GetWidget()) {
    wm::AddLimitedPreTargetHandlerForWindow(
        this, delegate_->GetWidget()->GetNativeWindow());
  }
}

void StatusAreaTooltipManager::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_EXITED) {
    Close();
    return;
  }

  if (event->type() != ui::ET_MOUSE_MOVED)
    return;

  gfx::Point point = event->location();
  views::View::ConvertPointFromWidget(delegate_, &point);
  views::View* view = delegate_->GetTooltipHandlerWithTooltipForPoint(point);
  const bool is_shelf_visible = shelf_ && shelf_->shelf_layout_manager() &&
                                shelf_->shelf_layout_manager()->IsVisible();

  timer_.Stop();
  if (IsVisible() && is_shelf_visible && bubble_->GetAnchorView() != view) {
    ShowTooltip(view);
  } else if (!IsVisible() && is_shelf_visible) {
    ShowTooltipWithDelay(view);
  } else {
    base::string16 text;
    view->GetTooltipText(gfx::Point(), &text);
    if (IsVisible() && text.empty())
      Close();
  }
}

void StatusAreaTooltipManager::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  // Close on any press events inside or outside the tooltip.
  if (event.type() == ui::ET_POINTER_DOWN)
    Close();
}

void StatusAreaTooltipManager::WillChangeVisibilityState(
    ShelfVisibilityState new_state) {
  if (new_state == SHELF_HIDDEN)
    Close();
}

void StatusAreaTooltipManager::OnAutoHideStateChanged(
    ShelfAutoHideState new_state) {
  if (new_state == SHELF_AUTO_HIDE_HIDDEN) {
    timer_.Stop();
    Close();
  }
}

void StatusAreaTooltipManager::ShowTooltipWithDelay(views::View* view) {
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromMilliseconds(kTooltipAppearanceDelay),
               base::Bind(&StatusAreaTooltipManager::ShowTooltip,
                          weak_factory_.GetWeakPtr(), view));
}

void StatusAreaTooltipManager::ShowTooltip(views::View* view) {
  timer_.Stop();
  Close();

  base::string16 text;
  view->GetTooltipText(gfx::Point(), &text);

  if (text.empty())
    return;

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

  bubble_ = new ShelfTooltipBubble(view, arrow, text,
                                   shelf_->shelf_widget()->GetNativeTheme(),
                                   false /* asymmetrical_border */);
  bubble_->GetWidget()->Show();
}

void StatusAreaTooltipManager::Close() {
  timer_.Stop();
  if (bubble_)
    bubble_->GetWidget()->Close();
  bubble_ = nullptr;
}

bool StatusAreaTooltipManager::IsVisible() const {
  return bubble_ && bubble_->GetWidget()->IsVisible();
}

}  // namespace ash
