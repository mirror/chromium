// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_item_delegate.h"

namespace ash {

ShelfItemDelegate::ShelfItemDelegate(const ash::ShelfID& shelf_id)
    : shelf_id_(shelf_id), binding_(this), image_set_by_controller_(false) {}

ShelfItemDelegate::~ShelfItemDelegate() {}

mojom::ShelfItemDelegatePtr ShelfItemDelegate::CreateInterfacePtrAndBind() {
  mojom::ShelfItemDelegatePtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

MenuItemList ShelfItemDelegate::GetAppMenuItems(int event_flags) {
  return MenuItemList();
}

MenuItemList ShelfItemDelegate::GetContextMenuItems(int64_t display_id) {
  LOG(ERROR) << "MSW ShelfItemDelegate::GetContextMenuItems";
  return MenuItemList();
}

AppWindowLauncherItemController*
ShelfItemDelegate::AsAppWindowLauncherItemController() {
  return nullptr;
}

void ShelfItemDelegate::ItemSelectedImpl(std::unique_ptr<ui::Event> event,
                                         int64_t display_id,
                                         ShelfLaunchSource source,
                                         ItemSelectedCallback callback) {
}

void ShelfItemDelegate::ItemSelected(std::unique_ptr<ui::Event> event,
                                     int64_t display_id,
                                     ShelfLaunchSource source,
                                     ItemSelectedCallback callback) {
  // Check for a right-click event to show the context menu.
  if (event && (event->type() == ui::ET_POINTER_DOWN &&
                event->flags() == ui::EF_RIGHT_MOUSE_BUTTON)) {
    LOG(ERROR) << "MSW ShelfItemDelegate::ItemSelected SHOW_CONTEXT_MENU";
    std::move(callback).Run(ash::SHELF_ACTION_SHOW_CONTEXT_MENU,
                            GetContextMenuItems(display_id));
    return;
  }

  ItemSelectedImpl(std::move(event), display_id, source, std::move(callback));
}

}  // namespace ash
