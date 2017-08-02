// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/launcher_context_menu.h"

#include <string>

#include "ash/public/cpp/shelf_model.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/wallpaper/wallpaper_delegate.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/chromeos/login/users/wallpaper/wallpaper_manager.h"
#include "chrome/browser/fullscreen.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/arc_launcher_context_menu.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
#include "chrome/browser/ui/ash/launcher/desktop_shell_launcher_context_menu.h"
#include "chrome/browser/ui/ash/launcher/extension_launcher_context_menu.h"
#include "chrome/browser/ui/ash/system_tray_client.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/session_manager/core/session_manager.h"
#include "content/public/common/context_menu_params.h"

namespace {

// Returns true if the user can modify the shelf's alignment.
// TODO(msw): Use a pref-modifiable check like auto-hide below? 
bool CanUserModifyShelfAlignment() {
  ash::LoginStatus status = SystemTrayClient::Get()->GetUserLoginStatus();
  return status == ash::LoginStatus::USER || status == ash::LoginStatus::OWNER;
}

// Returns true if the user can modify the shelf's auto-hide behavior.
bool CanUserModifyShelfAutoHideBehavior(const Profile* profile) {
  const std::string& pref = prefs::kShelfAutoHideBehaviorLocal;
  return profile->GetPrefs()->FindPreference(pref)->IsUserModifiable();
}

// Returns true if the user can modify the wallpaper.
// TODO(msw): Remove or consolidate WallpaperDelegate::CanOpenSetWallpaperPage
// function?
bool CanUserModifyWallpaper() {
  ash::LoginStatus status = SystemTrayClient::Get()->GetUserLoginStatus();
  if (status != ash::LoginStatus::USER && status != ash::LoginStatus::OWNER)
    return false;
  const user_manager::User* user =
      user_manager::UserManager::Get()->GetActiveUser();
  return user && !chromeos::WallpaperManager::Get()->IsPolicyControlled(
                     user->GetAccountId());
}

// TODO(msw): A menu that mimics ShelfAlignmentMenu from Chrome's perspective. 
class LauncherAlignmentMenu : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  explicit LauncherAlignmentMenu(int64_t display_id) : ui::SimpleMenuModel(this), display_id_(display_id) {
    const int align_group_id = 1;
    AddRadioItemWithStringId(
        MENU_ALIGN_LEFT, IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_LEFT, align_group_id);
    AddRadioItemWithStringId(MENU_ALIGN_BOTTOM,
                             IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_BOTTOM,
                             align_group_id);
    AddRadioItemWithStringId(
        MENU_ALIGN_RIGHT, IDS_ASH_SHELF_CONTEXT_MENU_ALIGN_RIGHT, align_group_id);
  }
  ~LauncherAlignmentMenu() override {}

  // ui::SimpleMenuModel::Delegate overrides:
  bool IsCommandIdChecked(int command_id) const override {
    Profile* profile = ChromeLauncherController::instance()->profile();
    const ash::ShelfAlignment alignment =
        GetShelfAlignmentPref(profile->GetPrefs(), display_id_);
    switch (alignment) {
      case ash::SHELF_ALIGNMENT_BOTTOM:
      case ash::SHELF_ALIGNMENT_BOTTOM_LOCKED:
        return command_id == MENU_ALIGN_BOTTOM;
      case ash::SHELF_ALIGNMENT_LEFT:
        return command_id == MENU_ALIGN_LEFT;
      case ash::SHELF_ALIGNMENT_RIGHT:
        return command_id == MENU_ALIGN_RIGHT;
    }
    return false;
  }
  bool IsCommandIdEnabled(int command_id) const override { return true; }
  void ExecuteCommand(int command_id, int event_flags) override {
    auto* prefs = ChromeLauncherController::instance()->profile()->GetPrefs();
    switch (static_cast<MenuItem>(command_id)) {
      case MENU_ALIGN_LEFT:
        // ash::Shell::Get()->metrics()->RecordUserMetricsAction(
        //     ash::UMA_SHELF_ALIGNMENT_SET_LEFT);
        SetShelfAlignmentPref(prefs, display_id_, ash::SHELF_ALIGNMENT_LEFT);
        break;
      case MENU_ALIGN_BOTTOM:
        // ash::Shell::Get()->metrics()->RecordUserMetricsAction(
        //     ash::UMA_SHELF_ALIGNMENT_SET_BOTTOM);
        SetShelfAlignmentPref(prefs, display_id_, ash::SHELF_ALIGNMENT_BOTTOM);
        break;
      case MENU_ALIGN_RIGHT:
        // ash::Shell::Get()->metrics()->RecordUserMetricsAction(
        //     ash::UMA_SHELF_ALIGNMENT_SET_RIGHT);
        SetShelfAlignmentPref(prefs, display_id_, ash::SHELF_ALIGNMENT_RIGHT);
        break;
    }
  }

 private:
  enum MenuItem {
    // Offset so as not to interfere with other menus.
    MENU_ALIGN_LEFT = 500,
    MENU_ALIGN_RIGHT,
    MENU_ALIGN_BOTTOM,
  };

  int64_t display_id_;

  DISALLOW_COPY_AND_ASSIGN(LauncherAlignmentMenu);
};

}  // namespace

// static
LauncherContextMenu* LauncherContextMenu::Create(
    ChromeLauncherController* controller,
    const ash::ShelfItem* item,
    int64_t display_id) {
  DCHECK(controller);
  // DCHECK(shelf);
  // Create DesktopShellLauncherContextMenu if no item is selected.
  if (!item || item->id.IsNull())
    return new DesktopShellLauncherContextMenu(controller, item, display_id);

  // Create ArcLauncherContextMenu if the item is an ARC app.
  if (arc::IsArcItem(controller->profile(), item->id.app_id))
    return new ArcLauncherContextMenu(controller, item, display_id);

  // Create ExtensionLauncherContextMenu for the item.
  return new ExtensionLauncherContextMenu(controller, item, display_id);
}

LauncherContextMenu::LauncherContextMenu(ChromeLauncherController* controller,
                                         const ash::ShelfItem* item,
                                         int64_t display_id)
    : ui::SimpleMenuModel(this),
      controller_(controller),
      item_(item ? *item : ash::ShelfItem()),
      display_id_(display_id) {}

LauncherContextMenu::~LauncherContextMenu() {}

bool LauncherContextMenu::IsCommandIdChecked(int command_id) const {
  if (command_id == MENU_AUTO_HIDE) {
    return GetShelfAutoHideBehaviorPref(controller_->profile()->GetPrefs(), display_id_) ==
        ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS;
  }
  DCHECK(command_id < MENU_ITEM_COUNT);
  return false;
}

bool LauncherContextMenu::IsCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case MENU_PIN:
      // Users cannot modify the pinned state of apps pinned by policy.
      return !item_.pinned_by_policy && (item_.type == ash::TYPE_PINNED_APP ||
                                         item_.type == ash::TYPE_APP);
    case MENU_CHANGE_WALLPAPER:
      return CanUserModifyWallpaper();
    case MENU_AUTO_HIDE:
      return CanUserModifyShelfAutoHideBehavior(controller_->profile());
    default:
      DCHECK(command_id < MENU_ITEM_COUNT);
      return true;
  }
}

void LauncherContextMenu::ExecuteCommand(int command_id, int event_flags) {
  switch (static_cast<MenuItem>(command_id)) {
    case MENU_OPEN_NEW:
      controller_->LaunchApp(item_.id, ash::LAUNCH_FROM_UNKNOWN, ui::EF_NONE, display_id_);
      break;
    case MENU_CLOSE:
      if (item_.type == ash::TYPE_DIALOG) {
        ash::ShelfItemDelegate* item_delegate =
            controller_->shelf_model()->GetShelfItemDelegate(item_.id);
        DCHECK(item_delegate);
        item_delegate->Close();
      } else {
        // TODO(simonhong): Use ShelfItemDelegate::Close().
        controller_->Close(item_.id);
      }
      base::RecordAction(base::UserMetricsAction("CloseFromContextMenu"));
      // if (ash::Shell::Get()
      //         ->tablet_mode_controller()
      //         ->IsTabletModeWindowManagerEnabled()) {
      //   base::RecordAction(
      //       base::UserMetricsAction("Tablet_WindowCloseFromContextMenu"));
      // }
      break;
    case MENU_PIN:
      if (controller_->IsAppPinned(item_.id.app_id))
        controller_->UnpinAppWithID(item_.id.app_id);
      else
        controller_->PinAppWithID(item_.id.app_id);
      break;
    case MENU_AUTO_HIDE:
      SetShelfAutoHideBehaviorPref(
          controller_->profile()->GetPrefs(), display_id_,
          GetShelfAutoHideBehaviorPref(controller_->profile()->GetPrefs(), display_id_) ==
                  ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS
              ? ash::SHELF_AUTO_HIDE_BEHAVIOR_NEVER
              : ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
      break;
    case MENU_ALIGNMENT_MENU:
      break;
    case MENU_CHANGE_WALLPAPER:
      chromeos::WallpaperManager::Get()->Open();
      break;
    default:
      NOTREACHED();
  }
}

void LauncherContextMenu::AddPinMenu() {
  // Expect a valid ShelfID to add pin/unpin menu item.
  DCHECK(!item_.id.IsNull());
  int menu_pin_string_id;
  switch (GetPinnableForAppID(item_.id.app_id, controller_->profile())) {
    case AppListControllerDelegate::PIN_EDITABLE:
      menu_pin_string_id = controller_->IsPinned(item_.id)
                               ? IDS_LAUNCHER_CONTEXT_MENU_UNPIN
                               : IDS_LAUNCHER_CONTEXT_MENU_PIN;
      break;
    case AppListControllerDelegate::PIN_FIXED:
      menu_pin_string_id = IDS_LAUNCHER_CONTEXT_MENU_PIN_ENFORCED_BY_POLICY;
      break;
    case AppListControllerDelegate::NO_PIN:
      return;
    default:
      NOTREACHED();
      return;
  }
  AddItemWithStringId(MENU_PIN, menu_pin_string_id);
}

void LauncherContextMenu::AddShelfOptionsMenu() {
  // In fullscreen, the launcher is either hidden or auto hidden depending
  // on the type of fullscreen. Do not show the auto-hide menu item while in
  // fullscreen per display because it is confusing when the preference appears
  // not to apply.
  if (!IsFullScreenMode(display_id_) &&
      CanUserModifyShelfAutoHideBehavior(controller_->profile())) {
    AddCheckItemWithStringId(MENU_AUTO_HIDE,
                             IDS_ASH_SHELF_CONTEXT_MENU_AUTO_HIDE);
  }
  if (CanUserModifyShelfAlignment() && 
      !session_manager::SessionManager::Get()->IsScreenLocked()) {
    shelf_alignment_menu_ = base::MakeUnique<LauncherAlignmentMenu>(display_id_);
    AddSubMenuWithStringId(MENU_ALIGNMENT_MENU,
                           IDS_ASH_SHELF_CONTEXT_MENU_POSITION,
                           shelf_alignment_menu_.get());
  }
  if (!controller_->profile()->IsGuestSession())
    AddItemWithStringId(MENU_CHANGE_WALLPAPER, IDS_AURA_SET_DESKTOP_WALLPAPER);
}

bool LauncherContextMenu::ExecuteCommonCommand(int command_id,
                                               int event_flags) {
  switch (command_id) {
    case MENU_OPEN_NEW:
    case MENU_CLOSE:
    case MENU_PIN:
    case MENU_AUTO_HIDE:
    case MENU_ALIGNMENT_MENU:
    case MENU_CHANGE_WALLPAPER:
      LauncherContextMenu::ExecuteCommand(command_id, event_flags);
      return true;
    default:
      return false;
  }
}
