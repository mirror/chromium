// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_item_delegate.h"

#include "ui/base/models/menu_model.h"

namespace ash {

namespace {

// TODO(msw): Formalize with enum traits? 
ash::mojom::MenuItem::Type ConvertItemTypeToMojo(ui::MenuModel::ItemType type) {
  switch(type) {
   case ui::MenuModel::TYPE_COMMAND:
     return ash::mojom::MenuItem::Type::ITEM;
   case ui::MenuModel::TYPE_CHECK:
     return ash::mojom::MenuItem::Type::CHECK;
   case ui::MenuModel::TYPE_RADIO:
     return ash::mojom::MenuItem::Type::RADIO;
   case ui::MenuModel::TYPE_SEPARATOR:
     return ash::mojom::MenuItem::Type::SEPARATOR;
   case ui::MenuModel::TYPE_BUTTON_ITEM:
     NOTIMPLEMENTED();
     return ash::mojom::MenuItem::Type::ITEM;
   case ui::MenuModel::TYPE_SUBMENU:
     return ash::mojom::MenuItem::Type::SUBMENU;
  }
  NOTREACHED();
  return ash::mojom::MenuItem::Type::ITEM;
}

// TODO(msw): Support button row items, multiple separator types, sublabels,
// minor text, dynamic items, label fonts, accelerators, visibility, etc.
ash::MenuItemList GetMenuItemsForMojo(ui::MenuModel* model) {
  DCHECK(model);
  ash::MenuItemList items;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->type = ConvertItemTypeToMojo(model->GetTypeAt(i));
    item->command_id = model->GetCommandIdAt(i);
    item->label = model->GetLabelAt(i);
    item->checked = model->IsItemCheckedAt(i);
    item->enabled = model->IsEnabledAt(i);
    item->radio_group_id = model->GetGroupIdAt(i);
    if (item->type == ash::mojom::MenuItem::Type::SUBMENU)
      item->submenu = GetMenuItemsForMojo(model->GetSubmenuModelAt(i));
    items.push_back(std::move(item));
  }
  return items;
}

// Attempts to activate the menu at the index with the given |command_id|.
// Returns true if the item was activated, false otherwise (ie. was not found).
bool ExecuteContextMenuCommand(ui::MenuModel* model, int64_t command_id) {
  DCHECK(model);
  for (int i = 0; i < model->GetItemCount(); ++i) {
    if (model->GetCommandIdAt(i) == command_id) {
      model->ActivatedAt(i);
      return true;
    } else if (model->GetTypeAt(i) == ui::MenuModel::TYPE_SUBMENU &&
               ExecuteContextMenuCommand(model->GetSubmenuModelAt(i),
                                         command_id)) {
      return true;
    }
  }
  return false;
}

}  // namespace

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

std::unique_ptr<ui::MenuModel> ShelfItemDelegate::GetContextMenu(int64_t display_id) {
  LOG(ERROR) << "MSW ShelfItemDelegate::GetContextMenu";
  return nullptr;
}

AppWindowLauncherItemController*
ShelfItemDelegate::AsAppWindowLauncherItemController() {
  return nullptr;
}

void ShelfItemDelegate::ItemSelectedImpl(std::unique_ptr<ui::Event> event,
                                         int64_t display_id,
                                         ShelfLaunchSource source,
                                         ItemSelectedCallback callback) {}

void ShelfItemDelegate::ExecuteCommandImpl(bool from_context_menu,
                                           int64_t command_id,
                                           int32_t event_flags,
                                           int64_t display_id) {}

void ShelfItemDelegate::ItemSelected(std::unique_ptr<ui::Event> event,
                                     int64_t display_id,
                                     ShelfLaunchSource source,
                                     ItemSelectedCallback callback) {
  // Help individual delegates show a context menu on right-click events.
  if (event && (event->type() == ui::ET_POINTER_DOWN &&
                event->flags() == ui::EF_RIGHT_MOUSE_BUTTON)) {
    LOG(ERROR) << "MSW ShelfItemDelegate::ItemSelected SHOW_CONTEXT_MENU";
    context_menu_ = GetContextMenu(display_id);
    if (context_menu_) {
      std::move(callback).Run(ash::SHELF_ACTION_SHOW_CONTEXT_MENU,
                              GetMenuItemsForMojo(context_menu_.get()));
      return;
    }
  }

  ItemSelectedImpl(std::move(event), display_id, source, std::move(callback));
}

void ShelfItemDelegate::ExecuteCommand(bool from_context_menu,
                                       int64_t command_id,
                                       int32_t event_flags,
                                       int64_t display_id) {
  LOG(ERROR) << "MSW ShelfItemDelegate::ExecuteCommand";
  if (from_context_menu) {
    LOG(ERROR) << "MSW ShelfItemDelegate::ExecuteCommand CONTEXT_MENU";
    DCHECK(context_menu_.get());
    ExecuteContextMenuCommand(context_menu_.get(), command_id);
    context_menu_.reset();
    return;
  }

  ExecuteCommandImpl(from_context_menu, command_id, event_flags, display_id);
}

}  // namespace ash
