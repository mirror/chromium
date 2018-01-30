// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_view_tooltip_manager.h"

#include "ash/public/cpp/config.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/wm/window_util.h"
#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/widget/widget.h"

namespace ash {

ShelfViewTooltipManager::ShelfViewTooltipManager(ShelfView* shelf_view)
    : ShelfTooltipManager(shelf_view->shelf()),
      shelf_view_(shelf_view),
      weak_factory_(this) {
  shelf_view_->shelf()->AddObserver(this);
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);
}

ShelfViewTooltipManager::~ShelfViewTooltipManager() {
  ShellPort::Get()->RemovePointerWatcher(this);
  shelf_view_->shelf()->RemoveObserver(this);
  aura::Window* window = nullptr;
  if (shelf_view_->GetWidget())
    window = shelf_view_->GetWidget()->GetNativeWindow();
  if (window)
    wm::RemoveLimitedPreTargetHandlerForWindow(this, window);
}

void ShelfViewTooltipManager::Init() {
  wm::AddLimitedPreTargetHandlerForWindow(
      this, shelf_view_->GetWidget()->GetNativeWindow());
}

views::View* ShelfViewTooltipManager::GetCurrentAnchorView() const {
  return bubble_ ? bubble_->GetAnchorView() : nullptr;
}

void ShelfViewTooltipManager::ShowTooltip(views::View* view) {
  ShowTooltipWithText(view, shelf_view_->GetTitleForView(view),
                      true /* asymmetrical_border */);
}

bool ShelfViewTooltipManager::ShouldShowTooltipForView(views::View* view) {
  Shelf* shelf = shelf_view_ ? shelf_view_->shelf() : nullptr;
  return shelf && shelf_view_->visible() &&
         shelf_view_->ShouldShowTooltipForView(view) &&
         (shelf->GetVisibilityState() == SHELF_VISIBLE ||
          (shelf->GetVisibilityState() == SHELF_AUTO_HIDE &&
           shelf->GetAutoHideState() == SHELF_AUTO_HIDE_SHOWN));
}

void ShelfViewTooltipManager::OnMouseEvent(ui::MouseEvent* event) {
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

}  // namespace ash
