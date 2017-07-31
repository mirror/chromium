// Copyright 2012 The Chromium Authors. All rights reserved.
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
#include "ash/wallpaper/wallpaper_delegate.h"
#include "ash/wm/maximize_mode/maximize_mode_controller.h"
#include "base/metrics/user_metrics.h"
#include "base/numerics/safe_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

namespace ash {

namespace {

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
          NOTIMPLEMENTED() << "Nested submenus are not supported.";
          break;
      }
    }
  }
  ~ShelfContextSubMenuModel() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfContextSubMenuModel);
};

// Returns true if the user can modify the shelf's alignment.
bool CanChangeShelfAlignment() {
  ash::LoginStatus status = Shell::Get()->session_controller()->login_status();
  return status == ash::LoginStatus::USER || status == ash::LoginStatus::OWNER;
}

  // TODO(msw): Cleanup Chrome's IsFullScreenMode? 
bool IsFullScreenMode(int64_t display_id) {
  auto* controller = Shell::GetRootWindowControllerWithDisplayId(display_id);
  return controller && controller->GetWindowForFullscreenMode();
}

// Returns true if the user can modify the shelf's auto-hide behavior.
bool CanChangeShelfAutoHideBehavior(int64_t display_id) {
  // In fullscreen, the shelf is either hidden or auto-hidden, depending on the
  // type of fullscreen. Do not show the auto-hide menu item while in fullscreen
  // because it is confusing when the preference appears not to apply.
  if (IsFullScreenMode(display_id))
    return false;
  // TODO(msw): Figure out how to do this in Ash... 
  // Profile* profile = ChromeLauncherController::instance()->profile();
  // const std::string& pref = prefs::kShelfAutoHideBehaviorLocal;
  // return profile->GetPrefs()->FindPreference(pref)->IsUserModifiable();
  return true; 
}

// Returns true if the user can modify the wallpaper.
// TODO(msw): Remove or consolidate WallpaperDelegate::CanOpenSetWallpaperPage
// function?
bool CanChangeWallpaper() {
  ash::LoginStatus status = Shell::Get()->session_controller()->login_status();
  if (status != ash::LoginStatus::USER && status != ash::LoginStatus::OWNER)
    return false;
  // TODO(msw): Figure out how to do this in Ash... CanOpenSetWallpaperPage? 
  // const user_manager::User* user =
  //     user_manager::UserManager::Get()->GetActiveUser();
  // return user && !chromeos::WallpaperManager::Get()->IsPolicyControlled(
  //                    user->GetAccountId());
  return true; 
}

// Add the context menu items to change shelf auto-hide and alignment settings
// and to change the wallpaper for the display with the given |display_id|.
void AddShelfContextMenuItems(ash::MenuItemList* menu, int64_t display_id) {
  auto* controller = Shell::GetRootWindowControllerWithDisplayId(display_id);
  Shelf* shelf = controller ? controller->shelf() : nullptr;
  if (!shelf)
    return;

  LOG(ERROR) << "MSW AddShelfContextMenuItems ASH A";
  if (CanChangeShelfAutoHideBehavior(display_id)) {
    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH B";
    DCHECK_NE(ash::LoginStatus::SUPERVISED,
              Shell::Get()->session_controller()->login_status());
    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->type = ash::mojom::MenuItem::Type::CHECK;
    item->command_id = ash::ShelfContextMenuItem::MENU_AUTO_HIDE;
    item->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_AUTO_HIDE);
    item->checked = shelf->auto_hide_behavior() ==
                    ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS;
    LOG(ERROR) << "MSW AddShelfContextMenuItems Autohide = " << shelf->auto_hide_behavior() << " checked? " << item->checked;
    item->enabled = true;
    menu->push_back(std::move(item));
  }
  if (CanChangeShelfAlignment()) {
    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH C";
    DCHECK_NE(ash::LoginStatus::SUPERVISED,
          Shell::Get()->session_controller()->login_status());
    ash::mojom::MenuItemPtr submenu(ash::mojom::MenuItem::New());
    submenu->type = ash::mojom::MenuItem::Type::SUBMENU;
    submenu->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_MENU;
    submenu->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_POSITION);
    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH D";
    submenu->submenu = ash::MenuItemList();
    submenu->enabled = true;
    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH E";

    // const uint32_t radio_group_id = 0;
    const ash::ShelfAlignment alignment = shelf->alignment();
    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH F";
    ash::mojom::MenuItemPtr left(ash::mojom::MenuItem::New());
    left->type = ash::mojom::MenuItem::Type::RADIO;
    left->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_LEFT;
    left->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_LEFT);
    left->checked = alignment == ash::SHELF_ALIGNMENT_LEFT;
    left->enabled = true;
    submenu->submenu->push_back(std::move(left));

    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH G";
    ash::mojom::MenuItemPtr right(ash::mojom::MenuItem::New());
    right->type = ash::mojom::MenuItem::Type::RADIO;
    right->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_RIGHT;
    right->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_RIGHT);
    right->checked = alignment == ash::SHELF_ALIGNMENT_RIGHT;
    right->enabled = true;
    submenu->submenu->push_back(std::move(right));

    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH H";
    ash::mojom::MenuItemPtr bottom(ash::mojom::MenuItem::New());
    bottom->type = ash::mojom::MenuItem::Type::RADIO;
    bottom->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_BOTTOM;
    bottom->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_BOTTOM);
    bottom->checked = alignment == ash::SHELF_ALIGNMENT_BOTTOM ||
                      alignment == ash::SHELF_ALIGNMENT_BOTTOM_LOCKED;
    bottom->enabled = true;
    submenu->submenu->push_back(std::move(bottom));

    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH I";
    menu->push_back(std::move(submenu));
  }
  if (CanChangeWallpaper()) {
    LOG(ERROR) << "MSW AddShelfContextMenuItems ASH J";
    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->command_id = ash::ShelfContextMenuItem::MENU_CHANGE_WALLPAPER;
    item->label = l10n_util::GetStringUTF16(IDS_AURA_SET_DESKTOP_WALLPAPER);
    item->enabled = true;
    menu->push_back(std::move(item));
  }
}

}  // namespace

ShelfContextMenuModel::ShelfContextMenuModel(
    // Shelf* shelf,
    // const ShelfItem& shelf_item,
    std::vector<mojom::MenuItemPtr> menu_items,
    ShelfItemDelegate* delegate,
    int64_t display_id)
    : ui::SimpleMenuModel(this),
      // shelf_(shelf),
      // shelf_item_(shelf_item),
      menu_items_(std::move(menu_items)),
      delegate_(delegate),
      display_id_(display_id) {

  // TODO(msw): Append some options handled by Ash.
  AddShelfContextMenuItems(&menu_items_, display_id);

  for (mojom::MenuItemPtr& item : menu_items_) {
    LOG(ERROR) << "MSW Adding item " << item->label << " " << item->type;
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
        AddSeparator(ui::SPACING_SEPARATOR);
        break;
      case mojom::MenuItem::Type::SUBMENU:
        DCHECK(!submenu_) << "Only one submenu is supported.";
        if (item->submenu.has_value()) {
          submenu_ = base::MakeUnique<ShelfContextSubMenuModel>(
              this, item->submenu.value());
          AddSubMenu(item->command_id, item->label, submenu_.get());
        }
        break;
    }
    // if (!item->image.isNull()) {
    //   SetIcon(GetIndexOfCommandId(item->command_id),
    //           gfx::Image::CreateFrom1xBitmap(item->image));
    // }
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
  // Have the shelf item delegate execute the context menu command.
  if (delegate_)
    delegate_->ExecuteCommand(true, command_id, event_flags, display_id_);
}

}  // namespace ash
