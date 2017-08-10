// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_item_delegate.h"

#include "ui/base/models/menu_model.h"

namespace ash {

namespace {

// Get the mojo menu item type from the MenuModel enum value.
mojom::MenuItem::Type GetItemTypeForMojo(ui::MenuModel::ItemType type) {
  switch (type) {
    case ui::MenuModel::TYPE_COMMAND:
      return mojom::MenuItem::Type::ITEM;
    case ui::MenuModel::TYPE_CHECK:
      return mojom::MenuItem::Type::CHECK;
    case ui::MenuModel::TYPE_RADIO:
      return mojom::MenuItem::Type::RADIO;
    case ui::MenuModel::TYPE_SEPARATOR:
      return mojom::MenuItem::Type::SEPARATOR;
    case ui::MenuModel::TYPE_BUTTON_ITEM:
      NOTIMPLEMENTED() << "Button menu items are not supported.";
      return mojom::MenuItem::Type::ITEM;
    case ui::MenuModel::TYPE_SUBMENU:
      return mojom::MenuItem::Type::SUBMENU;
  }
  NOTREACHED();
  return mojom::MenuItem::Type::ITEM;
}

// Get a serialized list of mojo MenuItemPtr objects to transport a menu model.
// NOTE: This does not support button items, some separator types, sublabels,
// minor text, dynamic items, label fonts, accelerators, visibility, etc.
MenuItemList GetMenuItemsForMojo(ui::MenuModel* model) {
  DCHECK(model);
  MenuItemList items;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    mojom::MenuItemPtr item(mojom::MenuItem::New());
    item->type = GetItemTypeForMojo(model->GetTypeAt(i));
    item->command_id = model->GetCommandIdAt(i);
    item->label = model->GetLabelAt(i);
    item->checked = model->IsItemCheckedAt(i);
    item->enabled = model->IsEnabledAt(i);
    item->radio_group_id = model->GetGroupIdAt(i);
    if (item->type == mojom::MenuItem::Type::SUBMENU)
      item->submenu = GetMenuItemsForMojo(model->GetSubmenuModelAt(i));
    items.push_back(std::move(item));
  }
  return items;
}

}  // namespace

ShelfItemDelegate::ShelfItemDelegate(const ShelfID& shelf_id)
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

std::unique_ptr<ui::MenuModel> ShelfItemDelegate::GetContextMenu(
    int64_t display_id) {
  return nullptr;
}

AppWindowLauncherItemController*
ShelfItemDelegate::AsAppWindowLauncherItemController() {
  return nullptr;
}

bool ShelfItemDelegate::WillShowContextMenu(const ui::Event* event,
                                            int64_t display_id,
                                            ItemSelectedCallback* callback) {
  // Help delegate subclasses show a context menu on right-click events.
  if (event && (event->type() == ui::ET_POINTER_DOWN &&
                event->flags() == ui::EF_RIGHT_MOUSE_BUTTON)) {
    context_menu_ = GetContextMenu(display_id);
    if (context_menu_) {
      std::move(*callback).Run(SHELF_ACTION_SHOW_CONTEXT_MENU,
                               GetMenuItemsForMojo(context_menu_.get()));
      return true;
    }
  }
  return false;
}

bool ShelfItemDelegate::ExecuteContextMenuCommand(int64_t command_id,
                                                  int32_t event_flags) {
  DCHECK(context_menu_);
  // Help delegate subclasses execute context menu items, may be on a sub-menu.
  ui::MenuModel* model = context_menu_.get();
  int index = -1;
  if (!ui::MenuModel::GetModelAndIndexForCommandId(command_id, &model, &index))
    return false;

  model->ActivatedAt(index, event_flags);
  return true;
}

}  // namespace ash
