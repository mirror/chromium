// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"

#include "ash/login_status.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/strings/grit/ash_strings.h"
#include "chrome/browser/chromeos/login/users/wallpaper/wallpaper_manager.h"
#include "chrome/browser/fullscreen.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/chrome_launcher_prefs.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "chrome/browser/ui/ash/system_tray_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Returns true if the user can modify the shelf's alignment.
bool CanChangeShelfAlignment() {
  ash::LoginStatus status = SystemTrayClient::Get()->GetUserLoginStatus();
  return status == ash::LoginStatus::USER || status == ash::LoginStatus::OWNER;
}

// Returns true if the user can modify the shelf's auto-hide behavior.
bool CanChangeShelfAutoHideBehavior(int64_t display_id) {
  // In fullscreen, the shelf is either hidden or auto-hidden, depending on the
  // type of fullscreen. Do not show the auto-hide menu item while in fullscreen
  // because it is confusing when the preference appears not to apply.
  if (IsFullScreenMode(display_id))
    return false;
  Profile* profile = ChromeLauncherController::instance()->profile();
  const std::string& pref = prefs::kShelfAutoHideBehaviorLocal;
  return profile->GetPrefs()->FindPreference(pref)->IsUserModifiable();
}

// Returns true if the user can modify the wallpaper.
// TODO(msw): Remove or consolidate WallpaperDelegate::CanOpenSetWallpaperPage
// function?
bool CanChangeWallpaper() {
  ash::LoginStatus status = SystemTrayClient::Get()->GetUserLoginStatus();
  if (status != ash::LoginStatus::USER && status != ash::LoginStatus::OWNER)
    return false;
  const user_manager::User* user =
      user_manager::UserManager::Get()->GetActiveUser();
  return user && !chromeos::WallpaperManager::Get()->IsPolicyControlled(
                     user->GetAccountId());
}

}  // namespace

bool IsBrowserFromActiveUser(Browser* browser) {
  // If running multi user mode with separate desktops, we have to check if the
  // browser is from the active user.
  if (chrome::MultiUserWindowManager::GetMultiProfileMode() !=
      chrome::MultiUserWindowManager::MULTI_PROFILE_MODE_SEPARATED)
    return true;
  return multi_user_util::IsProfileFromActiveUser(browser->profile());
}

const extensions::Extension* GetExtensionForAppID(const std::string& app_id,
                                                  Profile* profile) {
  return extensions::ExtensionRegistry::Get(profile)->GetExtensionById(
      app_id, extensions::ExtensionRegistry::EVERYTHING);
}

AppListControllerDelegate::Pinnable GetPinnableForAppID(
    const std::string& app_id,
    Profile* profile) {
  const base::ListValue* pref =
      profile->GetPrefs()->GetList(prefs::kPolicyPinnedLauncherApps);
  if (!pref)
    return AppListControllerDelegate::PIN_EDITABLE;
  // Pinned ARC apps policy defines the package name of the apps, that must
  // be pinned. All the launch activities of any package in policy are pinned.
  // In turn the input parameter to this function is app_id, which
  // is 32 chars hash. In case of ARC app this is a hash of
  // (package name + activity). This means that we must identify the package
  // from the hash, and check if this package is pinned by policy.
  const ArcAppListPrefs* const arc_prefs = ArcAppListPrefs::Get(profile);
  std::string arc_app_packege_name;
  if (arc_prefs) {
    std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
        arc_prefs->GetApp(app_id);
    if (app_info)
      arc_app_packege_name = app_info->package_name;
  }
  for (size_t index = 0; index < pref->GetSize(); ++index) {
    const base::DictionaryValue* app = nullptr;
    std::string app_id_or_package;
    if (pref->GetDictionary(index, &app) &&
        app->GetString(kPinnedAppsPrefAppIDPath, &app_id_or_package) &&
        (app_id == app_id_or_package ||
         arc_app_packege_name == app_id_or_package)) {
      return AppListControllerDelegate::PIN_FIXED;
    }
  }
  return AppListControllerDelegate::PIN_EDITABLE;
}

void AddPinContextMenuItem(ash::MenuItemList* menu, const ash::ShelfID& id) {
  LOG(ERROR) << "MSW AddPinContextMenuItem";
  ChromeLauncherController* controller = ChromeLauncherController::instance();
  ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
  item->command_id = ash::ShelfContextMenuItem::MENU_PIN;
  int menu_pin_string_id = IDS_LAUNCHER_CONTEXT_MENU_PIN_ENFORCED_BY_POLICY;
  switch (GetPinnableForAppID(id.app_id, controller->profile())) {
    case AppListControllerDelegate::PIN_EDITABLE:
      menu_pin_string_id = controller->IsPinned(id)
                               ? IDS_LAUNCHER_CONTEXT_MENU_UNPIN
                               : IDS_LAUNCHER_CONTEXT_MENU_PIN;
      break;
    case AppListControllerDelegate::PIN_FIXED:
      menu_pin_string_id = IDS_LAUNCHER_CONTEXT_MENU_PIN_ENFORCED_BY_POLICY;
      break;
    case AppListControllerDelegate::NO_PIN:
      NOTREACHED();
  }
  item->label = l10n_util::GetStringUTF16(menu_pin_string_id);
  const ash::ShelfItem* shelf_item = controller->GetItem(id);
  item->enabled = shelf_item && !shelf_item->pinned_by_policy &&
                  (shelf_item->type == ash::TYPE_PINNED_APP ||
                   shelf_item->type == ash::TYPE_APP);
  menu->push_back(std::move(item));
}

void AddShelfContextMenuItems(ash::MenuItemList* menu, int64_t display_id) {
  LOG(ERROR) << "MSW AddShelfContextMenuItems A";
  Profile* profile = ChromeLauncherController::instance()->profile();
  if (CanChangeShelfAutoHideBehavior(display_id)) {
    LOG(ERROR) << "MSW AddShelfContextMenuItems B";
    DCHECK(!profile->IsSupervised());
    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->type = ash::mojom::MenuItem::Type::CHECK;
    item->command_id = ash::ShelfContextMenuItem::MENU_AUTO_HIDE;
    item->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_AUTO_HIDE);
    item->checked =
        GetShelfAutoHideBehaviorPref(profile->GetPrefs(), display_id) ==
        ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS;
    item->enabled = true;
    menu->push_back(std::move(item));
  }
  if (CanChangeShelfAlignment()) {
    LOG(ERROR) << "MSW AddShelfContextMenuItems C";
    DCHECK(!profile->IsSupervised());
    ash::mojom::MenuItemPtr submenu(ash::mojom::MenuItem::New());
    submenu->type = ash::mojom::MenuItem::Type::SUBMENU;
    submenu->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_MENU;
    submenu->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_POSITION);
    LOG(ERROR) << "MSW AddShelfContextMenuItems D";
    submenu->submenu = ash::MenuItemList();
    submenu->enabled = true;
    LOG(ERROR) << "MSW AddShelfContextMenuItems E";

    // const uint32_t radio_group_id = 0;
    const ash::ShelfAlignment alignment =
        GetShelfAlignmentPref(profile->GetPrefs(), display_id);

    LOG(ERROR) << "MSW AddShelfContextMenuItems F";
    ash::mojom::MenuItemPtr left(ash::mojom::MenuItem::New());
    left->type = ash::mojom::MenuItem::Type::RADIO;
    left->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_LEFT;
    left->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_LEFT);
    left->checked = alignment == ash::SHELF_ALIGNMENT_LEFT;
    left->enabled = true;
    submenu->submenu->push_back(std::move(left));

    LOG(ERROR) << "MSW AddShelfContextMenuItems G";
    ash::mojom::MenuItemPtr right(ash::mojom::MenuItem::New());
    right->type = ash::mojom::MenuItem::Type::RADIO;
    right->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_RIGHT;
    right->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_RIGHT);
    right->checked = alignment == ash::SHELF_ALIGNMENT_RIGHT;
    right->enabled = true;
    submenu->submenu->push_back(std::move(right));

    LOG(ERROR) << "MSW AddShelfContextMenuItems H";
    ash::mojom::MenuItemPtr bottom(ash::mojom::MenuItem::New());
    bottom->type = ash::mojom::MenuItem::Type::RADIO;
    bottom->command_id = ash::ShelfContextMenuItem::MENU_ALIGNMENT_BOTTOM;
    bottom->label =
        l10n_util::GetStringUTF16(IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_BOTTOM);
    bottom->checked = alignment == ash::SHELF_ALIGNMENT_BOTTOM ||
                      alignment == ash::SHELF_ALIGNMENT_BOTTOM_LOCKED;
    bottom->enabled = true;
    submenu->submenu->push_back(std::move(bottom));

    LOG(ERROR) << "MSW AddShelfContextMenuItems I";
    menu->push_back(std::move(submenu));
  }
  if (CanChangeWallpaper()) {
    LOG(ERROR) << "MSW AddShelfContextMenuItems J";
    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->command_id = ash::ShelfContextMenuItem::MENU_CHANGE_WALLPAPER;
    item->label = l10n_util::GetStringUTF16(IDS_AURA_SET_DESKTOP_WALLPAPER);
    item->enabled = true;
    menu->push_back(std::move(item));
  }
}

void ExecuteContextMenuCommand(const ash::ShelfID& id,
                               uint32_t command_id,
                               int64_t display_id) {
  LOG(ERROR) << "MSW ExecuteContextMenuCommand A";
  ChromeLauncherController* controller = ChromeLauncherController::instance();
  const ash::ShelfItem* item = controller->GetItem(id);
  PrefService* prefs = controller->profile()->GetPrefs();
  switch (static_cast<ash::ShelfContextMenuItem>(command_id)) {
    case ash::MENU_OPEN_NEW:
      controller->LaunchApp(id, ash::LAUNCH_FROM_UNKNOWN, ui::EF_NONE,
                            display_id);
      break;
    case ash::MENU_CLOSE:
      if (item && item->type == ash::TYPE_DIALOG) {
        controller->shelf_model()->GetShelfItemDelegate(id)->Close();
      } else {
        // TODO(simonhong): Use ShelfItemDelegate::Close().
        controller->Close(id);
      }
      // base::RecordAction(base::UserMetricsAction("CloseFromContextMenu"));
      // if (Shell::Get()
      //         ->maximize_mode_controller()
      //         ->IsMaximizeModeWindowManagerEnabled()) {
      //   base::RecordAction(
      //       base::UserMetricsAction("Tablet_WindowCloseFromContextMenu"));
      // }
      break;
    case ash::MENU_PIN:
      if (controller->IsAppPinned(id.app_id))
        controller->UnpinAppWithID(id.app_id);
      else
        controller->PinAppWithID(id.app_id);
      break;
    case ash::MENU_AUTO_HIDE:
      SetShelfAutoHideBehaviorPref(
          prefs, display_id,
          GetShelfAutoHideBehaviorPref(prefs, display_id) ==
                  ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS
              ? ash::SHELF_AUTO_HIDE_BEHAVIOR_NEVER
              : ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
      break;
    case ash::MENU_ALIGNMENT_MENU:
      break;
    case ash::MENU_ALIGNMENT_LEFT:
      SetShelfAlignmentPref(prefs, display_id, ash::SHELF_ALIGNMENT_LEFT);
      break;
    case ash::MENU_ALIGNMENT_RIGHT:
      SetShelfAlignmentPref(prefs, display_id, ash::SHELF_ALIGNMENT_RIGHT);
      break;
    case ash::MENU_ALIGNMENT_BOTTOM:
      SetShelfAlignmentPref(prefs, display_id, ash::SHELF_ALIGNMENT_BOTTOM);
      break;
    case ash::MENU_CHANGE_WALLPAPER:
      chromeos::WallpaperManager::Get()->Open();
      break;
    default:
      NOTREACHED();
  }
}
