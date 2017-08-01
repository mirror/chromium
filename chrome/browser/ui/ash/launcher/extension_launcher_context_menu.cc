// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/extension_launcher_context_menu.h"

#include "base/bind.h"
#include "chrome/browser/extensions/context_menu_matcher.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/launcher/browser_shortcut_launcher_item_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/common/context_menu_params.h"
#include "extensions/browser/extension_prefs.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// A helper used to filter which menu items added by the extension are shown.
bool MenuItemHasLauncherContext(const extensions::MenuItem* item) {
  return item->contexts().Contains(extensions::MenuItem::LAUNCHER);
}

// A helper to get the launch type for the extension item.
// TODO(msw): Rename... 
extensions::LaunchType GetLaunchTypeMSW(ash::ShelfID shelf_id) {
  Profile* profile = ChromeLauncherController::instance()->profile();
  const extensions::Extension* extension =
      GetExtensionForAppID(shelf_id.app_id, profile);

  // An extension can be unloaded/updated/unavailable at any time.
  if (!extension)
    return extensions::LAUNCH_TYPE_DEFAULT;

  return extensions::GetLaunchType(
      extensions::ExtensionPrefs::Get(profile), extension);
}

// A helper to set the launch type for the extension item.
// void SetLaunchType(ash::ShelfID shelf_id, extensions::LaunchType launch_type) {
//   Profile* profile = ChromeLauncherController::instance()->profile();
//   extensions::SetLaunchType(profile, shelf_id.app_id, launch_type);
// }

}  // namespace

ExtensionLauncherContextMenu::ExtensionLauncherContextMenu(
    ChromeLauncherController* controller,
    const ash::ShelfItem* item,
    ash::Shelf* shelf)
    : LauncherContextMenu(controller, item, shelf) {
  Init();
}

ExtensionLauncherContextMenu::~ExtensionLauncherContextMenu() {}

// static
ash::MenuItemList ExtensionLauncherContextMenu::GetMenuItems(
    const ash::ShelfItem* item) {
  LOG(ERROR) << "MSW GetMenuItems A";
  LOG(ERROR) << "MSW GetMenuItems TEST MENU INSTANCE CTOR START";
  ExtensionLauncherContextMenu msw(ChromeLauncherController::instance(), item, nullptr);
  
  
  LOG(ERROR) << "MSW GetMenuItems TEST MENU INSTANCE CTOR END";

  ash::MenuItemList items;
  ChromeLauncherController* controller = ChromeLauncherController::instance();
  // std::unique_ptr<extensions::ContextMenuMatcher> extension_items;
  // extension_items.reset(new extensions::ContextMenuMatcher(
  //     controller->profile(), this, this,
  //     base::Bind(MenuItemHasLauncherContext)));
  if (item->type == ash::TYPE_PINNED_APP || item->type == ash::TYPE_APP) {
    LOG(ERROR) << "MSW GetMenuItems B";
    // V1 apps can be started from the menu - but V2 apps should not.
    if (!controller->IsPlatformApp(item->id)) {
      LOG(ERROR) << "MSW GetMenuItems C";
      ash::mojom::MenuItemPtr open(ash::mojom::MenuItem::New());
      open->command_id = ash::ShelfContextMenuItem::MENU_OPEN_NEW;
      switch (GetLaunchTypeMSW(item->id)) {
        case extensions::LAUNCH_TYPE_PINNED:
        case extensions::LAUNCH_TYPE_REGULAR:
          open->label = l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_TAB);
        case extensions::LAUNCH_TYPE_FULLSCREEN:
        case extensions::LAUNCH_TYPE_WINDOW:
          open->label = l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_WINDOW);
        default:
          NOTREACHED();
      }
      open->enabled = true;
      items.push_back(std::move(open));

      ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
      separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
      items.push_back(std::move(separator));
    }

    AddPinContextMenuItem(&items, item->id);

    if (controller->IsOpen(item->id)) {
      LOG(ERROR) << "MSW GetMenuItems D";
      ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
      close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
      close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
      close->enabled = true;
      items.push_back(std::move(close));
    }

    if (!controller->IsPlatformApp(item->id) &&
        item->type == ash::TYPE_PINNED_APP) {
      LOG(ERROR) << "MSW GetMenuItems E";
      ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
      separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
      items.push_back(std::move(separator));

      const int launch_type = GetLaunchTypeMSW(item->id);
      if (extensions::util::IsNewBookmarkAppsEnabled()) {
        LOG(ERROR) << "MSW GetMenuItems F";
        // With bookmark apps enabled, hosted apps launch in a window by
        // default. This menu item is re-interpreted as a single, toggle-able
        // option to launch the hosted app as a tab.
        ash::mojom::MenuItemPtr window(ash::mojom::MenuItem::New());
        window->type = ash::mojom::MenuItem::Type::CHECK;
        window->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_WINDOW;
        window->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
        window->enabled = true;
        window->checked = launch_type == extensions::LAUNCH_TYPE_WINDOW;
        items.push_back(std::move(window));
      } else {
        LOG(ERROR) << "MSW GetMenuItems G";
        ash::mojom::MenuItemPtr regular(ash::mojom::MenuItem::New());
        regular->type = ash::mojom::MenuItem::Type::CHECK;
        regular->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_REGULAR_TAB;
        regular->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_REGULAR);
        regular->enabled = true;
        regular->checked = launch_type == extensions::LAUNCH_TYPE_REGULAR;
        items.push_back(std::move(regular));

        ash::mojom::MenuItemPtr pinned(ash::mojom::MenuItem::New());
        pinned->type = ash::mojom::MenuItem::Type::CHECK;
        pinned->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_PINNED_TAB;
        pinned->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_PINNED);
        pinned->enabled = true;
        pinned->checked = launch_type == extensions::LAUNCH_TYPE_PINNED;
        items.push_back(std::move(pinned));

        ash::mojom::MenuItemPtr window(ash::mojom::MenuItem::New());
        window->type = ash::mojom::MenuItem::Type::CHECK;
        window->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_WINDOW;
        window->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
        window->enabled = true;
        window->checked = launch_type == extensions::LAUNCH_TYPE_WINDOW;
        items.push_back(std::move(window));

        // Even though the launch type is Full Screen it is more accurately
        // described as Maximized in Ash.
        ash::mojom::MenuItemPtr fullscreen(ash::mojom::MenuItem::New());
        fullscreen->type = ash::mojom::MenuItem::Type::CHECK;
        fullscreen->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_FULLSCREEN;
        fullscreen->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_MAXIMIZED);
        fullscreen->enabled = true;
        fullscreen->checked = launch_type == extensions::LAUNCH_TYPE_FULLSCREEN;
        items.push_back(std::move(fullscreen));
      }
    }
  } else if (item->type == ash::TYPE_BROWSER_SHORTCUT) {
    LOG(ERROR) << "MSW GetMenuItems H";
    ash::mojom::MenuItemPtr window(ash::mojom::MenuItem::New());
    window->command_id = ash::ShelfContextMenuItem::MENU_NEW_WINDOW;
    window->label = l10n_util::GetStringUTF16(IDS_APP_LIST_NEW_WINDOW);
    window->enabled = true;
    items.push_back(std::move(window));

    if (!controller->profile()->IsGuestSession()) {
      LOG(ERROR) << "MSW GetMenuItems I";
      ash::mojom::MenuItemPtr incognito(ash::mojom::MenuItem::New());
      incognito->command_id = ash::ShelfContextMenuItem::MENU_NEW_INCOGNITO_WINDOW;
      incognito->label = l10n_util::GetStringUTF16(IDS_APP_LIST_NEW_INCOGNITO_WINDOW);
      incognito->enabled = true;
      items.push_back(std::move(incognito));
    }
    if (!BrowserShortcutLauncherItemController(controller->shelf_model())
             .IsListOfActiveBrowserEmpty()) {
      LOG(ERROR) << "MSW GetMenuItems J";
      ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
      close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
      close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
      close->enabled = true;
      items.push_back(std::move(close));
    }
  } else if (item->type == ash::TYPE_DIALOG) {
    LOG(ERROR) << "MSW GetMenuItems K";
    ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
    close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
    close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
    close->enabled = true;
    items.push_back(std::move(close));
  } else if (controller->IsOpen(item->id)) {
    LOG(ERROR) << "MSW GetMenuItems L";
    ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
    close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
    close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
    close->enabled = true;
    items.push_back(std::move(close));
  }

  LOG(ERROR) << "MSW GetMenuItems M";
  ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
  separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
  items.push_back(std::move(separator));

  if (item->type == ash::TYPE_PINNED_APP || item->type == ash::TYPE_APP) {
    LOG(ERROR) << "MSW GetMenuItems N";
    const extensions::MenuItem::ExtensionKey app_key(item->id.app_id);
    if (!app_key.empty()) {
      LOG(ERROR) << "MSW GetMenuItems O";
      // int index = 0;
      // extension_items->AppendExtensionItems(app_key, base::string16(), &index,
      //                                       false);  // is_action_menu
      // ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
      // separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
      // items.push_back(std::move(separator));
    }
  }
  // AddShelfOptionsMenu(); 
  LOG(ERROR) << "MSW GetMenuItems Z";
  return items;
}

void ExtensionLauncherContextMenu::Init() {
  extension_items_.reset(new extensions::ContextMenuMatcher(
      controller()->profile(), this, this,
      base::Bind(MenuItemHasLauncherContext)));
  if (item().type == ash::TYPE_PINNED_APP || item().type == ash::TYPE_APP) {
    // V1 apps can be started from the menu - but V2 apps should not.
    if (!controller()->IsPlatformApp(item().id)) {
      AddItem(MENU_OPEN_NEW, base::string16());
      AddSeparator(ui::NORMAL_SEPARATOR);
    }

    AddPinMenu();

    if (controller()->IsOpen(item().id))
      AddItemWithStringId(MENU_CLOSE, IDS_LAUNCHER_CONTEXT_MENU_CLOSE);

    if (!controller()->IsPlatformApp(item().id) &&
        item().type == ash::TYPE_PINNED_APP) {
      AddSeparator(ui::NORMAL_SEPARATOR);
      if (extensions::util::IsNewBookmarkAppsEnabled()) {
        // With bookmark apps enabled, hosted apps launch in a window by
        // default. This menu item is re-interpreted as a single, toggle-able
        // option to launch the hosted app as a tab.
        AddCheckItemWithStringId(LAUNCH_TYPE_WINDOW,
                                 IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
      } else {
        AddCheckItemWithStringId(LAUNCH_TYPE_REGULAR_TAB,
                                 IDS_APP_CONTEXT_MENU_OPEN_REGULAR);
        AddCheckItemWithStringId(LAUNCH_TYPE_PINNED_TAB,
                                 IDS_APP_CONTEXT_MENU_OPEN_PINNED);
        AddCheckItemWithStringId(LAUNCH_TYPE_WINDOW,
                                 IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
        // Even though the launch type is Full Screen it is more accurately
        // described as Maximized in Ash.
        AddCheckItemWithStringId(LAUNCH_TYPE_FULLSCREEN,
                                 IDS_APP_CONTEXT_MENU_OPEN_MAXIMIZED);
      }
    }
  } else if (item().type == ash::TYPE_BROWSER_SHORTCUT) {
    LOG(ERROR) << "MSW WTF?!?!?!?!!";
    AddItemWithStringId(MENU_NEW_WINDOW, IDS_APP_LIST_NEW_WINDOW);
    if (!controller()->profile()->IsGuestSession()) {
      AddItemWithStringId(MENU_NEW_INCOGNITO_WINDOW,
                          IDS_APP_LIST_NEW_INCOGNITO_WINDOW);
    }
    if (!BrowserShortcutLauncherItemController(controller()->shelf_model())
             .IsListOfActiveBrowserEmpty()) {
      AddItem(MENU_CLOSE,
              l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE));
    }
  } else if (item().type == ash::TYPE_DIALOG) {
    AddItemWithStringId(MENU_CLOSE, IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  } else if (controller()->IsOpen(item().id)) {
    AddItemWithStringId(MENU_CLOSE, IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  }
  AddSeparator(ui::NORMAL_SEPARATOR);
  if (item().type == ash::TYPE_PINNED_APP || item().type == ash::TYPE_APP) {
    const extensions::MenuItem::ExtensionKey app_key(item().id.app_id);
    if (!app_key.empty()) {
      int index = 0;
      LOG(ERROR) << "MSW APPENDING EXTENSION ITEMS!!!!!! count before: " << GetItemCount();
      extension_items_->AppendExtensionItems(app_key, base::string16(), &index,
                                             false);  // is_action_menu
      LOG(ERROR) << "MSW APPENDING EXTENSION ITEMS!!!!!! count after: " << GetItemCount();
      AddSeparator(ui::NORMAL_SEPARATOR);
    }
  }
  AddShelfOptionsMenu();
}

bool ExtensionLauncherContextMenu::IsItemForCommandIdDynamic(
    int command_id) const {
  return command_id == MENU_OPEN_NEW;
}

base::string16 ExtensionLauncherContextMenu::GetLabelForCommandId(
    int command_id) const {
  if (command_id == MENU_OPEN_NEW) {
    if (controller()->IsPlatformApp(item().id))
      return l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_WINDOW);
    switch (GetLaunchType()) {
      case extensions::LAUNCH_TYPE_PINNED:
      case extensions::LAUNCH_TYPE_REGULAR:
        return l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_TAB);
      case extensions::LAUNCH_TYPE_FULLSCREEN:
      case extensions::LAUNCH_TYPE_WINDOW:
        return l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_WINDOW);
      default:
        NOTREACHED();
        return base::string16();
    }
  }
  NOTREACHED();
  return base::string16();
}

bool ExtensionLauncherContextMenu::IsCommandIdChecked(int command_id) const {
  switch (command_id) {
    case LAUNCH_TYPE_PINNED_TAB:
      return GetLaunchType() == extensions::LAUNCH_TYPE_PINNED;
    case LAUNCH_TYPE_REGULAR_TAB:
      return GetLaunchType() == extensions::LAUNCH_TYPE_REGULAR;
    case LAUNCH_TYPE_WINDOW:
      return GetLaunchType() == extensions::LAUNCH_TYPE_WINDOW;
    case LAUNCH_TYPE_FULLSCREEN:
      return GetLaunchType() == extensions::LAUNCH_TYPE_FULLSCREEN;
    default:
      if (command_id < MENU_ITEM_COUNT)
        return LauncherContextMenu::IsCommandIdChecked(command_id);
      return (extension_items_ &&
              extension_items_->IsCommandIdChecked(command_id));
  }
}

bool ExtensionLauncherContextMenu::IsCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case MENU_NEW_WINDOW:
      // "Normal" windows are not allowed when incognito is enforced.
      return IncognitoModePrefs::GetAvailability(
                 controller()->profile()->GetPrefs()) !=
             IncognitoModePrefs::FORCED;
    case MENU_NEW_INCOGNITO_WINDOW:
      // Incognito windows are not allowed when incognito is disabled.
      return IncognitoModePrefs::GetAvailability(
                 controller()->profile()->GetPrefs()) !=
             IncognitoModePrefs::DISABLED;
    default:
      if (command_id < MENU_ITEM_COUNT)
        return LauncherContextMenu::IsCommandIdEnabled(command_id);
      return (extension_items_ &&
              extension_items_->IsCommandIdEnabled(command_id));
  }
}

void ExtensionLauncherContextMenu::ExecuteCommand(int command_id,
                                                  int event_flags) {
  if (ExecuteCommonCommand(command_id, event_flags))
    return;
  switch (static_cast<MenuItem>(command_id)) {
    case LAUNCH_TYPE_PINNED_TAB:
      SetLaunchType(extensions::LAUNCH_TYPE_PINNED);
      break;
    case LAUNCH_TYPE_REGULAR_TAB:
      SetLaunchType(extensions::LAUNCH_TYPE_REGULAR);
      break;
    case LAUNCH_TYPE_WINDOW: {
      extensions::LaunchType launch_type = extensions::LAUNCH_TYPE_WINDOW;
      // With bookmark apps enabled, hosted apps can only toggle between
      // LAUNCH_WINDOW and LAUNCH_REGULAR.
      if (extensions::util::IsNewBookmarkAppsEnabled()) {
        launch_type = GetLaunchType() == extensions::LAUNCH_TYPE_WINDOW
                          ? extensions::LAUNCH_TYPE_REGULAR
                          : extensions::LAUNCH_TYPE_WINDOW;
      }
      SetLaunchType(launch_type);
      break;
    }
    case LAUNCH_TYPE_FULLSCREEN:
      SetLaunchType(extensions::LAUNCH_TYPE_FULLSCREEN);
      break;
    case MENU_NEW_WINDOW:
      chrome::NewEmptyWindow(controller()->profile());
      break;
    case MENU_NEW_INCOGNITO_WINDOW:
      chrome::NewEmptyWindow(controller()->profile()->GetOffTheRecordProfile());
      break;
    default:
      if (extension_items_) {
        extension_items_->ExecuteCommand(command_id, nullptr, nullptr,
                                         content::ContextMenuParams());
      }
  }
}

extensions::LaunchType ExtensionLauncherContextMenu::GetLaunchType() const {
  const extensions::Extension* extension =
      GetExtensionForAppID(item().id.app_id, controller()->profile());

  // An extension can be unloaded/updated/unavailable at any time.
  if (!extension)
    return extensions::LAUNCH_TYPE_DEFAULT;

  return extensions::GetLaunchType(
      extensions::ExtensionPrefs::Get(controller()->profile()), extension);
}

void ExtensionLauncherContextMenu::SetLaunchType(extensions::LaunchType type) {
  extensions::SetLaunchType(controller()->profile(), item().id.app_id, type);
}
