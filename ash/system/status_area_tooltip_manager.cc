// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/status_area_tooltip_manager.h"

#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shelf/shelf_tooltip_bubble.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell_port.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/wm/window_util.h"

namespace ash {

StatusAreaTooltipManager::StatusAreaTooltipManager(
    StatusAreaWidgetDelegate* delegate,
    Shelf* shelf)
    : ShelfTooltipBase(shelf),
      delegate_(delegate),
      shelf_(shelf),
      weak_factory_(this) {
  shelf_->AddObserver(this);
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);
}

StatusAreaTooltipManager::~StatusAreaTooltipManager() {
  ShellPort::Get()->RemovePointerWatcher(this);
  shelf_->RemoveObserver(this);
  aura::Window* window = nullptr;
  if (delegate_->GetWidget())
    window = delegate_->GetWidget()->GetNativeWindow();

  if (window)
    wm::RemoveLimitedPreTargetHandlerForWindow(this, window);
}

void StatusAreaTooltipManager::Init() {
  if (delegate_->GetWidget()) {
    wm::AddLimitedPreTargetHandlerForWindow(
        this, delegate_->GetWidget()->GetNativeWindow());
  }
}

void StatusAreaTooltipManager::ShowTooltip(views::View* view) {
  timer_.Stop();
  Close();

  if (!ShouldShowTooltipForView(view))
    return;

  bubble_ = new ShelfTooltipBubble(view, GetBubbleArrow(), tooltip_text_,
                                   shelf_->shelf_widget()->GetNativeTheme(),
                                   false /* asymmetrical_border */);
  bubble_->GetWidget()->Show();
}

void StatusAreaTooltipManager::ShowTooltipWithDelay(views::View* view) {
  if (ShouldShowTooltipForView(view)) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(timer_delay_),
                 base::Bind(&StatusAreaTooltipManager::ShowTooltip,
                            weak_factory_.GetWeakPtr(), view));
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
  views::View* view = delegate_->GetTooltipHandlerWithTextForPoint(point);
  const bool should_show = ShouldShowTooltipForView(view);

  timer_.Stop();
  if (IsVisible() && should_show && bubble_->GetAnchorView() != view) {
    ShowTooltip(view);
  } else if (!IsVisible() && should_show) {
    ShowTooltipWithDelay(view);
  } else {
    if (IsVisible() && tooltip_text_.empty())
      Close();
  }
}

bool StatusAreaTooltipManager::ShouldShowTooltipForView(views::View* view) {
  if (!view)
    return false;

  tooltip_text_.clear();
  view->SetTextToGivenTooltip(&tooltip_text_);
  if (tooltip_text_.empty())
    return false;

  return shelf_ && shelf_->shelf_layout_manager() &&
         shelf_->shelf_layout_manager()->IsVisible();
}

}  // namespace ash
