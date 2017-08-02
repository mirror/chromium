// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_context_menu_model.h"

#include <string>

#include "ash/public/cpp/shelf_item_delegate.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ash/wallpaper/wallpaper_delegate.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/metrics/user_metrics.h"
#include "base/numerics/safe_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

using l10n_util::GetStringUTF16;

namespace ash {

namespace {

// The command ids for locally-handled shelf and wallpaper context menu items.
enum AshMenuItem {
  // Offset so as not to interfere with other menus.
  MENU_LOCAL_START = 500,
  MENU_AUTO_HIDE = MENU_LOCAL_START,
  MENU_ALIGNMENT_MENU,
  MENU_ALIGNMENT_LEFT,
  MENU_ALIGNMENT_RIGHT,
  MENU_ALIGNMENT_BOTTOM,
  MENU_CHANGE_WALLPAPER,
  MENU_LOCAL_END
};

// Find a menu item by command id; returns a stub item if no match was found.
const mojom::MenuItemPtr& GetItem(const std::vector<mojom::MenuItemPtr>& items,
                                  int command_id) {
  static const mojom::MenuItemPtr item_not_found(ash::mojom::MenuItem::New());
  for (const mojom::MenuItemPtr& item : items) {
    if (item->command_id == base::checked_cast<uint32_t>(command_id))
      return item;
    if (item->type == mojom::MenuItem::Type::SUBMENU &&
        item->submenu.has_value()) {
      const mojom::MenuItemPtr& submenu_item =
          GetItem(item->submenu.value(), command_id);
      if (submenu_item->command_id == base::checked_cast<uint32_t>(command_id))
        return submenu_item;
    }
  }
  return item_not_found;
}

// A shelf context submenu model; used for shelf alignment.
class ShelfContextSubMenuModel : public ui::SimpleMenuModel {
 public:
  ShelfContextSubMenuModel(ShelfContextMenuModel* owner,
                           const std::vector<mojom::MenuItemPtr>& menu_items)
      : ui::SimpleMenuModel(owner) {
    for (const mojom::MenuItemPtr& item : menu_items) {
      switch (item->type) {
        case mojom::MenuItem::Type::ITEM:
          AddItem(item->command_id, item->label);
          break;
        case mojom::MenuItem::Type::CHECK:
          AddItem(item->command_id, item->label);
          break;
        case mojom::MenuItem::Type::RADIO:
          AddRadioItem(item->command_id, item->label, item->radio_group_id);
          break;
        case mojom::MenuItem::Type::SEPARATOR:
          AddSeparator(ui::NORMAL_SEPARATOR);
          break;
        case mojom::MenuItem::Type::SUBMENU:
          NOTREACHED() << "Nested submenus are not supported.";
          break;
      }
    }
  }
  ~ShelfContextSubMenuModel() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfContextSubMenuModel);
};

// Returns true if the display is showing a fullscreen window.
// NOTE: This duplicates the functionality of Chrome's IsFullScreenMode.
bool IsFullScreenMode(int64_t display_id) {
  auto* controller = Shell::GetRootWindowControllerWithDisplayId(display_id);
  return controller && controller->GetWindowForFullscreenMode();
}

// Add the context menu items to change shelf auto-hide and alignment settings
// and to change the wallpaper for the display with the given |display_id|.
void AddLocalMenuItems(ash::MenuItemList* menu, int64_t display_id) {
  // Only allow shelf and wallpaper modifications by the owner or user.
  ash::LoginStatus status = Shell::Get()->session_controller()->login_status();
  if (status != ash::LoginStatus::USER && status != ash::LoginStatus::OWNER)
    return;

  auto* controller = Shell::GetRootWindowControllerWithDisplayId(display_id);
  Shelf* shelf = controller ? controller->shelf() : nullptr;
  if (!shelf)
    return;

  // In fullscreen, the shelf is either hidden or auto-hidden, depending on the
  // type of fullscreen. Do not show the auto-hide menu item while in fullscreen
  // because it is confusing when the preference appears not to apply.
  if (!IsFullScreenMode(display_id)) {
    ash::mojom::MenuItemPtr auto_hide(ash::mojom::MenuItem::New());
    auto_hide->type = ash::mojom::MenuItem::Type::CHECK;
    auto_hide->command_id = MENU_AUTO_HIDE;
    auto_hide->label = GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_AUTO_HIDE);
    auto_hide->checked =
        shelf->auto_hide_behavior() == ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS;
    auto_hide->enabled = true;
    menu->push_back(std::move(auto_hide));
  }

  ash::mojom::MenuItemPtr alignment(ash::mojom::MenuItem::New());
  alignment->type = ash::mojom::MenuItem::Type::SUBMENU;
  alignment->command_id = MENU_ALIGNMENT_MENU;
  alignment->label = GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_POSITION);
  alignment->submenu = ash::MenuItemList();
  alignment->enabled = true;

  ash::mojom::MenuItemPtr left(ash::mojom::MenuItem::New());
  left->type = ash::mojom::MenuItem::Type::RADIO;
  left->command_id = MENU_ALIGNMENT_LEFT;
  left->label = GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_LEFT);
  left->checked = shelf->alignment() == ash::SHELF_ALIGNMENT_LEFT;
  left->enabled = true;
  alignment->submenu->push_back(std::move(left));

  ash::mojom::MenuItemPtr right(ash::mojom::MenuItem::New());
  right->type = ash::mojom::MenuItem::Type::RADIO;
  right->command_id = MENU_ALIGNMENT_RIGHT;
  right->label = GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_RIGHT);
  right->checked = shelf->alignment() == ash::SHELF_ALIGNMENT_RIGHT;
  right->enabled = true;
  alignment->submenu->push_back(std::move(right));

  ash::mojom::MenuItemPtr bottom(ash::mojom::MenuItem::New());
  bottom->type = ash::mojom::MenuItem::Type::RADIO;
  bottom->command_id = MENU_ALIGNMENT_BOTTOM;
  bottom->label = GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_BOTTOM);
  bottom->checked = shelf->alignment() == ash::SHELF_ALIGNMENT_BOTTOM ||
                    shelf->alignment() == ash::SHELF_ALIGNMENT_BOTTOM_LOCKED;
  bottom->enabled = true;
  alignment->submenu->push_back(std::move(bottom));

  menu->push_back(std::move(alignment));

  if (Shell::Get()->wallpaper_delegate()->CanOpenSetWallpaperPage()) {
    ash::mojom::MenuItemPtr wallpaper(ash::mojom::MenuItem::New());
    wallpaper->command_id = MENU_CHANGE_WALLPAPER;
    wallpaper->label = GetStringUTF16(IDS_AURA_SET_DESKTOP_WALLPAPER);
    wallpaper->enabled = true;
    menu->push_back(std::move(wallpaper));
  }
}

}  // namespace

ShelfContextMenuModel::ShelfContextMenuModel(
    std::vector<mojom::MenuItemPtr> menu_items,
    ShelfItemDelegate* delegate,
    int64_t display_id)
    : ui::SimpleMenuModel(this),
      menu_items_(std::move(menu_items)),
      delegate_(delegate),
      display_id_(display_id) {
  // Append some menu items that are handled locally by Ash.
  AddLocalMenuItems(&menu_items_, display_id);

  for (mojom::MenuItemPtr& item : menu_items_) {
    switch (item->type) {
      case mojom::MenuItem::Type::ITEM:
        AddItem(item->command_id, item->label);
        break;
      case mojom::MenuItem::Type::CHECK:
        AddCheckItem(item->command_id, item->label);
        break;
      case mojom::MenuItem::Type::RADIO:
        AddRadioItem(item->command_id, item->label, item->radio_group_id);
        break;
      case mojom::MenuItem::Type::SEPARATOR:
        AddSeparator(ui::NORMAL_SEPARATOR);
        break;
      case mojom::MenuItem::Type::SUBMENU:
        DCHECK(!submenu_) << "Only one submenu is supported.";
        if (!submenu_ && item->submenu.has_value()) {
          submenu_ = base::MakeUnique<ShelfContextSubMenuModel>(
              this, item->submenu.value());
          AddSubMenu(item->command_id, item->label, submenu_.get());
        }
        break;
    }
    if (!item->image.isNull()) {
      SetIcon(GetIndexOfCommandId(item->command_id),
              gfx::Image::CreateFrom1xBitmap(item->image));
    }
  }
}

ShelfContextMenuModel::~ShelfContextMenuModel() {}

bool ShelfContextMenuModel::IsCommandIdChecked(int command_id) const {
  return GetItem(menu_items_, command_id)->checked;
}

bool ShelfContextMenuModel::IsCommandIdEnabled(int command_id) const {
  return GetItem(menu_items_, command_id)->enabled;
}

void ShelfContextMenuModel::ExecuteCommand(int command_id, int event_flags) {
  DCHECK(IsCommandIdEnabled(command_id));
  auto* controller = Shell::GetRootWindowControllerWithDisplayId(display_id_);
  Shelf* shelf = controller ? controller->shelf() : nullptr;
  DCHECK(shelf);

  if (command_id >= MENU_LOCAL_START && command_id < MENU_LOCAL_END) {
    switch (command_id) {
      case MENU_AUTO_HIDE:
        shelf->SetAutoHideBehavior(shelf->auto_hide_behavior() ==
                                           ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS
                                       ? ash::SHELF_AUTO_HIDE_BEHAVIOR_NEVER
                                       : ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
        break;
      case MENU_ALIGNMENT_LEFT:
        shelf->SetAlignment(SHELF_ALIGNMENT_LEFT);
        break;
      case MENU_ALIGNMENT_RIGHT:
        shelf->SetAlignment(SHELF_ALIGNMENT_RIGHT);
        break;
      case MENU_ALIGNMENT_BOTTOM:
        shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM);
        break;
      case MENU_CHANGE_WALLPAPER:
        Shell::Get()->wallpaper_controller()->OpenSetWallpaperPage();
        break;
      default:
        NOTREACHED();
    }
  }

  // Have the shelf item delegate execute the context menu command.
  if (delegate_)
    delegate_->ExecuteCommand(true, command_id, event_flags, display_id_);
}

}  // namespace ash
