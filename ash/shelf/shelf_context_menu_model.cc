// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_context_menu_model.h"

#include <string>

#include "ash/public/cpp/shelf_item_delegate.h"
// #include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/wallpaper/wallpaper_delegate.h"
#include "ash/wm/maximize_mode/maximize_mode_controller.h"
#include "base/metrics/user_metrics.h"
#include "base/numerics/safe_conversions.h"
// #include "chrome/browser/chromeos/login/users/wallpaper/wallpaper_manager.h"
// #include "chrome/browser/fullscreen.h"
// #include "chrome/browser/profiles/profile.h"
// #include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
// #include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
// #include "chrome/browser/ui/ash/launcher/arc_launcher_context_menu.h"
// #include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
// #include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
// #include "chrome/browser/ui/ash/launcher/desktop_shell_launcher_context_menu.h"
// #include "chrome/browser/ui/ash/launcher/extension_launcher_context_menu.h"
// #include "chrome/common/pref_names.h"
// #include "chrome/grit/generated_resources.h"
// #include "components/prefs/pref_service.h"
// #include "components/session_manager/core/session_manager.h"
// #include "content/public/common/context_menu_params.h"
#include "ui/gfx/image/image.h"

#include "ash/strings/grit/ash_strings.h" 

namespace ash {

namespace {

// Find a menu item by command id; returns a stub item if no match was found.
const mojom::MenuItemPtr& GetItem(const std::vector<mojom::MenuItemPtr>& items,
                                  int command_id) {
  static const mojom::MenuItemPtr item_not_found(ash::mojom::MenuItem::New());
  for (const mojom::MenuItemPtr& item : items) {
    if (item->command_id == base::checked_cast<uint32_t>(command_id))
      return item;
    if (item->type == mojom::MenuItem::Type::SUBMENU && item->submenu.has_value()) {
      const mojom::MenuItemPtr& submenu_item = GetItem(item->submenu.value(), command_id);
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
                           const std::vector<mojom::MenuItemPtr>& menu_items) : ui::SimpleMenuModel(owner) {
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
          AddSeparator(ui::SPACING_SEPARATOR);
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

}  // namespace

ShelfContextMenuModel::ShelfContextMenuModel(
    // Shelf* shelf, 
    // const ShelfItem& shelf_item, 
    std::vector<mojom::MenuItemPtr> menu_items,
    ShelfItemDelegate* delegate)
    : ui::SimpleMenuModel(this),
      // shelf_(shelf), 
      // shelf_item_(shelf_item), 
      menu_items_(std::move(menu_items)),
      delegate_(delegate) {
  for (mojom::MenuItemPtr& item : menu_items_) {
    LOG(ERROR) << "MSW Adding item " << item->label << " " << item->type;
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
        AddSeparator(ui::SPACING_SEPARATOR);
        break;
      case mojom::MenuItem::Type::SUBMENU:
        DCHECK(!submenu_) << "Only one submenu is supported.";
        if (item->submenu.has_value()) {
          submenu_ = base::MakeUnique<ShelfContextSubMenuModel>(this, item->submenu.value());
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
  // Have the delegate execute its own custom command id for the given item.
  if (delegate_)
    delegate_->ExecuteCommand(command_id, event_flags);

  // switch (static_cast<ShelfContextMenuItem>(command_id)) {
  //   case MENU_OPEN_NEW:
  //     // controller_->LaunchApp(item_.id, LAUNCH_FROM_UNKNOWN, ui::EF_NONE,
  //     //                        display::Screen::GetScreen()
  //     //                            ->GetDisplayNearestWindow(
  //     //                                shelf_->shelf_widget()->GetNativeWindow())
  //     //                            .id());
  //     break;
  //   case MENU_CLOSE:
  //     if (shelf_item_.type == TYPE_DIALOG) {
  //       // ShelfItemDelegate* item_delegate =
  //       //     controller_->shelf_model()->GetShelfItemDelegate(item_.id);
  //       // DCHECK(item_delegate);
  //       // item_delegate->Close();
  //     } else {
  //       // TODO(simonhong): Use ShelfItemDelegate::Close().
  //       // controller_->Close(item_.id);
  //     }
  //     base::RecordAction(base::UserMetricsAction("CloseFromContextMenu"));
  //     if (Shell::Get()
  //             ->maximize_mode_controller()
  //             ->IsMaximizeModeWindowManagerEnabled()) {
  //       base::RecordAction(
  //           base::UserMetricsAction("Tablet_WindowCloseFromContextMenu"));
  //     }
  //     break;
  //   case MENU_PIN:
  //     // if (controller_->IsAppPinned(item_.id.app_id))
  //     //   controller_->UnpinAppWithID(item_.id.app_id);
  //     // else
  //     //   controller_->PinAppWithID(item_.id.app_id);
  //     break;
  //   case MENU_AUTO_HIDE:
  //     shelf_->SetAutoHideBehavior(shelf_->auto_hide_behavior() ==
  //                                         SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS
  //                                     ? SHELF_AUTO_HIDE_BEHAVIOR_NEVER
  //                                     : SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  //     break;
  //   case MENU_ALIGNMENT_MENU:
  //     break;
  //   case MENU_CHANGE_WALLPAPER:
  //     // chromeos::WallpaperManager::Get()->Open();
  //     break;
  //   default:
  //     NOTREACHED();
  // }
}

}  // namespace ash
