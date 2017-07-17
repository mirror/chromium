// Copyright (c) 2012 The Chromium Authors. All rights reserved.
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

// Base class for a context menu which is shown for items in the shelf, or shown
// when right clicking on the desktop background wallpaper area.
class ShelfContextMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  // Menu item command ids, used by subclasses and tests.
  enum MenuItem {
    MENU_OPEN_NEW,
    MENU_CLOSE,
    MENU_PIN,
    LAUNCH_TYPE_PINNED_TAB,
    LAUNCH_TYPE_REGULAR_TAB,
    LAUNCH_TYPE_FULLSCREEN,
    LAUNCH_TYPE_WINDOW,
    MENU_AUTO_HIDE,
    MENU_NEW_WINDOW,
    MENU_NEW_INCOGNITO_WINDOW,
    MENU_ALIGNMENT_MENU,
    MENU_CHANGE_WALLPAPER,
    MENU_ITEM_COUNT
  };

  ShelfContextMenuModel(Shelf* shelf, const ShelfItem& item, std::vector<mojom::MenuItemPtr> items);
  ~ShelfContextMenuModel() override;

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsItemForCommandIdDynamic(int command_id) const override;
  base::string16 GetLabelForCommandId(int command_id) const override;
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 protected:
  const ShelfItem& item() const { return item_; }

  // Add menu item for pin/unpin.
  void AddPinMenu();

  // Add common shelf options items, e.g. autohide, alignment, and wallpaper.
  void AddShelfOptionsMenu();

  // Helper method to execute common commands. Returns true if handled.
  bool ExecuteCommonCommand(int command_id, int event_flags);

 private:
  Shelf* shelf_;
  ShelfItem item_;
  ShelfAlignmentMenu shelf_alignment_menu_;

  // The list of menu items as returned from the shelf item's controller.
  std::vector<mojom::MenuItemPtr> items_;

  DISALLOW_COPY_AND_ASSIGN(ShelfContextMenuModel);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_CONTEXT_MENU_MODEL_H_
