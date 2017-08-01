// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_
#define ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_

#include "ash/public/cpp/shelf_item.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "ash/shelf/shelf_alignment_menu.h"
#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"

namespace ash {

class Shelf;
class ShelfItemDelegate;

// A context menu shown for shelf items, the shelf itself, or the desktop area.
// TODO(msw): Don't use this for the desktop background? 
class ShelfContextMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  // Creates a context menu model with |menu_items| from the given |delegate|.
  ShelfContextMenuModel(std::vector<mojom::MenuItemPtr> menu_items,
                        ShelfItemDelegate* delegate,
                        int64_t display_id);
  ~ShelfContextMenuModel() override;

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  std::vector<mojom::MenuItemPtr> menu_items_;
  ShelfItemDelegate* delegate_;
  // TODO(msw): Support multiple/nested submenus.
  std::unique_ptr<ui::MenuModel> submenu_;
  const int64_t display_id_;

  DISALLOW_COPY_AND_ASSIGN(ShelfContextMenuModel);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_
