// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/status_area_tooltip_manager.h"

#include "ash/public/cpp/config.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/wm/window_util.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace ash {

StatusAreaTooltipManager::StatusAreaTooltipManager(
    StatusAreaWidgetDelegate* delegate,
    Shelf* shelf)
    : ShelfTooltipManager(shelf),
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
  ShowTooltipWithText(view, tooltip_text_, false /* asymmetrical_border */);
}

bool StatusAreaTooltipManager::ShouldShowTooltipForView(views::View* view) {
  // Should not show tooltip for a null view.
  if (!view)
    return false;

  // Should not show tooltip for a view that didn't override
  // SetTextToGivenTooltip to set non-empty tooltip text.
  tooltip_text_.clear();
  view->SetTextToGivenTooltip(&tooltip_text_);
  if (tooltip_text_.empty())
    return false;

  // Should not show tooltip if shelf and its contents are not visible on the
  // screen.
  return shelf_ && shelf_->shelf_layout_manager() &&
         shelf_->shelf_layout_manager()->IsVisible();
}

void StatusAreaTooltipManager::OnMouseEvent(ui::MouseEvent* event) {
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
  views::View::ConvertPointFromWidget(delegate_, &point);
  views::View* view = delegate_->GetTooltipHandlerWithTextForPoint(point);
  const bool should_show = ShouldShowTooltipForView(view);

  timer_.Stop();
  if (IsVisible() && should_show && bubble_->GetAnchorView() != view) {
    ShowTooltip(view);
  } else if (!IsVisible() && should_show) {
    ShowTooltipWithDelay(view);
  } else if (IsVisible() && tooltip_text_.empty()) {
    Close();
  }
}

}  // namespace ash
