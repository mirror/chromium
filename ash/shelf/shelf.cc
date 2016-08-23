// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf.h"

#include <algorithm>
#include <cmath>

#include "ash/aura/wm_window_aura.h"
#include "ash/common/shelf/shelf_delegate.h"
#include "ash/common/shelf/shelf_item_delegate.h"
#include "ash/common/shelf/shelf_model.h"
#include "ash/common/shelf/shelf_navigator.h"
#include "ash/common/shelf/shelf_view.h"
#include "ash/common/shell_window_ids.h"
#include "ash/common/wm_lookup.h"
#include "ash/common/wm_shell.h"
#include "ash/common/wm_window_property.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

Shelf::Shelf(WmShelf* wm_shelf,
             ShelfView* shelf_view,
             ShelfWidget* shelf_widget)
    : wm_shelf_(wm_shelf),
      shelf_widget_(shelf_widget),
      shelf_view_(shelf_view),
      shelf_locking_manager_(wm_shelf) {
  DCHECK(wm_shelf_);
  DCHECK(shelf_view_);
  DCHECK(shelf_widget_);
}

Shelf::~Shelf() {
  WmShell::Get()->shelf_delegate()->OnShelfDestroyed(this);
}

// static
Shelf* Shelf::ForPrimaryDisplay() {
  return Shelf::ForWindow(WmShell::Get()->GetPrimaryRootWindow());
}

// static
Shelf* Shelf::ForWindow(const WmWindow* window) {
  const aura::Window* aura_window = WmWindowAura::GetAuraWindow(window);
  ShelfWidget* shelf_widget =
      RootWindowController::ForWindow(aura_window)->shelf_widget();
  return shelf_widget ? shelf_widget->shelf() : nullptr;
}

void Shelf::SetAlignment(ShelfAlignment alignment) {
  if (alignment_ == alignment)
    return;

  if (shelf_locking_manager_.is_locked() &&
      alignment != SHELF_ALIGNMENT_BOTTOM_LOCKED) {
    shelf_locking_manager_.set_stored_alignment(alignment);
    return;
  }

  alignment_ = alignment;
  shelf_view_->OnShelfAlignmentChanged();
  shelf_widget_->OnShelfAlignmentChanged();
  WmShell::Get()->shelf_delegate()->OnShelfAlignmentChanged(this);
  WmShell::Get()->NotifyShelfAlignmentChanged(
      WmLookup::Get()->GetWindowForWidget(shelf_widget_)->GetRootWindow());
  // ShelfLayoutManager will resize the shelf.
}

void Shelf::SetAutoHideBehavior(ShelfAutoHideBehavior auto_hide_behavior) {
  if (auto_hide_behavior_ == auto_hide_behavior)
    return;

  auto_hide_behavior_ = auto_hide_behavior;
  WmShell::Get()->shelf_delegate()->OnShelfAutoHideBehaviorChanged(this);
  WmShell::Get()->NotifyShelfAutoHideBehaviorChanged(
      WmLookup::Get()->GetWindowForWidget(shelf_widget_)->GetRootWindow());
}

ShelfAutoHideState Shelf::GetAutoHideState() const {
  return shelf_widget_->shelf_layout_manager()->auto_hide_state();
}

ShelfVisibilityState Shelf::GetVisibilityState() const {
  return shelf_widget_->shelf_layout_manager()->visibility_state();
}

gfx::Rect Shelf::GetScreenBoundsOfItemIconForWindow(WmWindow* window) {
  ShelfID id = window->GetIntProperty(WmWindowProperty::SHELF_ID);
  gfx::Rect bounds(shelf_view_->GetIdealBoundsOfItemIcon(id));
  gfx::Point screen_origin;
  views::View::ConvertPointToScreen(shelf_view_, &screen_origin);
  return gfx::Rect(screen_origin.x() + bounds.x(),
                   screen_origin.y() + bounds.y(), bounds.width(),
                   bounds.height());
}

void Shelf::UpdateIconPositionForWindow(WmWindow* window) {
  WmWindow* shelf_window = WmLookup::Get()->GetWindowForWidget(shelf_widget_);
  shelf_view_->UpdatePanelIconPosition(
      window->GetIntProperty(WmWindowProperty::SHELF_ID),
      shelf_window->ConvertRectFromScreen(window->GetBoundsInScreen())
          .CenterPoint());
}

void Shelf::ActivateShelfItem(int index) {
  // We pass in a keyboard event which will then trigger a switch to the
  // next item if the current one is already active.
  ui::KeyEvent event(ui::ET_KEY_RELEASED,
                     ui::VKEY_UNKNOWN,  // The actual key gets ignored.
                     ui::EF_NONE);

  ShelfModel* shelf_model = WmShell::Get()->shelf_model();
  const ShelfItem& item = shelf_model->items()[index];
  ShelfItemDelegate* item_delegate = shelf_model->GetShelfItemDelegate(item.id);
  item_delegate->ItemSelected(event);
}

void Shelf::CycleWindowLinear(CycleDirection direction) {
  int item_index =
      GetNextActivatedItemIndex(*WmShell::Get()->shelf_model(), direction);
  if (item_index >= 0)
    ActivateShelfItem(item_index);
}

AppListButton* Shelf::GetAppListButton() const {
  return shelf_view_->GetAppListButton();
}

void Shelf::LaunchAppIndexAt(int item_index) {
  ShelfModel* shelf_model = WmShell::Get()->shelf_model();
  const ShelfItems& items = shelf_model->items();
  int item_count = shelf_model->item_count();
  int indexes_left = item_index >= 0 ? item_index : item_count;
  int found_index = -1;

  // Iterating until we have hit the index we are interested in which
  // is true once indexes_left becomes negative.
  for (int i = 0; i < item_count && indexes_left >= 0; i++) {
    if (items[i].type != TYPE_APP_LIST) {
      found_index = i;
      indexes_left--;
    }
  }

  // There are two ways how found_index can be valid: a.) the nth item was
  // found (which is true when indexes_left is -1) or b.) the last item was
  // requested (which is true when index was passed in as a negative number).
  if (found_index >= 0 && (indexes_left == -1 || item_index < 0)) {
    // Then set this one as active (or advance to the next item of its kind).
    ActivateShelfItem(found_index);
  }
}

gfx::Rect Shelf::GetVisibleItemsBoundsInScreen() const {
  return shelf_view_->GetVisibleItemsBoundsInScreen();
}

app_list::ApplicationDragAndDropHost* Shelf::GetDragAndDropHostForAppList() {
  return shelf_view_;
}

void Shelf::UpdateShelfItemBackground(int alpha) {
  shelf_view_->UpdateShelfItemBackground(alpha);
}

}  // namespace ash
