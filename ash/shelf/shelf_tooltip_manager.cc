// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_tooltip_manager.h"

#include "ash/public/cpp/config.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_tooltip_bubble.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/wm/window_util.h"
#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/wm/core/window_animations.h"

namespace ash {

ShelfTooltipManager::ShelfTooltipManager(ShelfView* shelf_view)
    : ShelfTooltipBase(shelf_view->shelf()),
      shelf_view_(shelf_view),
      weak_factory_(this) {
  shelf_view_->shelf()->AddObserver(this);
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);
}

ShelfTooltipManager::~ShelfTooltipManager() {
  ShellPort::Get()->RemovePointerWatcher(this);
  shelf_view_->shelf()->RemoveObserver(this);
  aura::Window* window = nullptr;
  if (shelf_view_->GetWidget())
    window = shelf_view_->GetWidget()->GetNativeWindow();
  if (window)
    wm::RemoveLimitedPreTargetHandlerForWindow(this, window);
}

void ShelfTooltipManager::Init() {
  wm::AddLimitedPreTargetHandlerForWindow(
      this, shelf_view_->GetWidget()->GetNativeWindow());
}

views::View* ShelfTooltipManager::GetCurrentAnchorView() const {
  return bubble_ ? bubble_->GetAnchorView() : nullptr;
}

void ShelfTooltipManager::ShowTooltip(views::View* view) {
  timer_.Stop();
  if (bubble_) {
    // Cancel the hiding animation to hide the old bubble immediately.
    ::wm::SetWindowVisibilityAnimationTransition(
        bubble_->GetWidget()->GetNativeWindow(), ::wm::ANIMATE_NONE);
    Close();
  }

  if (!ShouldShowTooltipForView(view))
    return;

  base::string16 text = shelf_view_->GetTitleForView(view);
  bubble_ = new ShelfTooltipBubble(
      view, GetBubbleArrow(), text,
      shelf_view_->shelf()->shelf_widget()->GetNativeTheme(),
      true /* asymmetrical_border */);
  aura::Window* window = bubble_->GetWidget()->GetNativeWindow();
  ::wm::SetWindowVisibilityAnimationType(
      window, ::wm::WINDOW_VISIBILITY_ANIMATION_TYPE_VERTICAL);
  ::wm::SetWindowVisibilityAnimationTransition(window, ::wm::ANIMATE_HIDE);
  bubble_->GetWidget()->Show();
}

void ShelfTooltipManager::ShowTooltipWithDelay(views::View* view) {
  if (ShouldShowTooltipForView(view)) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(timer_delay_),
                 base::Bind(&ShelfTooltipManager::ShowTooltip,
                            weak_factory_.GetWeakPtr(), view));
  }
}

void ShelfTooltipManager::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_EXITED) {
    Close();
    return;
  }

  if (event->type() != ui::ET_MOUSE_MOVED)
    return;

  // A workaround for crbug.com/756163, likely not needed as Mus/Mash matures.
  if (Shell::GetAshConfig() != Config::CLASSIC && event->location().IsOrigin())
    return;

  gfx::Point point = event->location();
  views::View::ConvertPointFromWidget(shelf_view_, &point);
  views::View* view = shelf_view_->GetTooltipHandlerForPoint(point);
  const bool should_show = ShouldShowTooltipForView(view);

  timer_.Stop();
  if (IsVisible() && should_show && bubble_->GetAnchorView() != view)
    ShowTooltip(view);
  else if (!IsVisible() && should_show)
    ShowTooltipWithDelay(view);
  else if (IsVisible() && shelf_view_->ShouldHideTooltip(point))
    Close();
}

bool ShelfTooltipManager::ShouldShowTooltipForView(views::View* view) {
  Shelf* shelf = shelf_view_ ? shelf_view_->shelf() : nullptr;
  return shelf && shelf_view_->visible() &&
         shelf_view_->ShouldShowTooltipForView(view) &&
         (shelf->GetVisibilityState() == SHELF_VISIBLE ||
          (shelf->GetVisibilityState() == SHELF_AUTO_HIDE &&
           shelf->GetAutoHideState() == SHELF_AUTO_HIDE_SHOWN));
}

}  // namespace ash
