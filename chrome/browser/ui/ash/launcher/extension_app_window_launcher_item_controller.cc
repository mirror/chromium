// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/extension_app_window_launcher_item_controller.h"

#include "ash/public/cpp/shelf_item_delegate.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/shelf/shelf_context_menu_model.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/extensions/context_menu_matcher.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/launcher/browser_shortcut_launcher_item_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
#include "chrome/browser/ui/ash/launcher/extension_launcher_context_menu.h"
#include "chrome/grit/generated_resources.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "extensions/browser/extension_prefs.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event.h"
#include "ui/gfx/image/image.h"
#include "ui/wm/core/window_animations.h"

// namespace {

// // A helper used to filter which menu items added by the extension are shown.
// bool MenuItemHasLauncherContext(const extensions::MenuItem* item) {
//   return item->contexts().Contains(extensions::MenuItem::LAUNCHER);
// }

// // A helper to get the launch type for the extension item.
// extensions::LaunchType GetLaunchType(ash::ShelfID shelf_id) {
//   Profile* profile = ChromeLauncherController::instance()->profile();
//   const extensions::Extension* extension =
//       GetExtensionForAppID(shelf_id.app_id, profile);

//   // An extension can be unloaded/updated/unavailable at any time.
//   if (!extension)
//     return extensions::LAUNCH_TYPE_DEFAULT;

//   return extensions::GetLaunchType(
//       extensions::ExtensionPrefs::Get(profile), extension);
// }

// // A helper to set the launch type for the extension item.
// void SetLaunchType(ash::ShelfID shelf_id, extensions::LaunchType launch_type) {
//   Profile* profile = ChromeLauncherController::instance()->profile();
//   extensions::SetLaunchType(profile, shelf_id.app_id, launch_type);
// }

// }  // namespace

ExtensionAppWindowLauncherItemController::
    ExtensionAppWindowLauncherItemController(const ash::ShelfID& shelf_id)
    : AppWindowLauncherItemController(shelf_id) {}

ExtensionAppWindowLauncherItemController::
    ~ExtensionAppWindowLauncherItemController() {}

void ExtensionAppWindowLauncherItemController::AddAppWindow(
    extensions::AppWindow* app_window) {
  DCHECK(!app_window->window_type_is_panel());
  AddWindow(app_window->GetBaseWindow());
}

ash::MenuItemList ExtensionAppWindowLauncherItemController::GetAppMenuItems(
    int event_flags) {
  ash::MenuItemList items;
  extensions::AppWindowRegistry* app_window_registry =
      extensions::AppWindowRegistry::Get(
          ChromeLauncherController::instance()->profile());

  uint32_t window_index = 0;
  for (const ui::BaseWindow* window : windows()) {
    extensions::AppWindow* app_window =
        app_window_registry->GetAppWindowForNativeWindow(
            window->GetNativeWindow());
    DCHECK(app_window);

    ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
    item->command_id = window_index;
    item->label = app_window->GetTitle();

    // Use the app's web contents favicon, or the app window's icon.
    favicon::FaviconDriver* favicon_driver =
        favicon::ContentFaviconDriver::FromWebContents(
            app_window->web_contents());
    gfx::Image icon = favicon_driver->GetFavicon();
    if (icon.IsEmpty()) {
      const gfx::ImageSkia* app_icon = nullptr;
      if (app_window->GetNativeWindow()) {
        app_icon = app_window->GetNativeWindow()->GetProperty(
            aura::client::kAppIconKey);
      }
      if (app_icon && !app_icon->isNull())
        icon = gfx::Image(*app_icon);
    }
    if (!icon.IsEmpty())
      item->image = *icon.ToSkBitmap();
    items.push_back(std::move(item));
    ++window_index;
  }
  return items;
}

ash::MenuItemList ExtensionAppWindowLauncherItemController::GetContextMenuItems(
    int64_t display_id) {
  ChromeLauncherController* controller = ChromeLauncherController::instance();
  const ash::ShelfItem* item = controller->GetItem(shelf_id());
  return ExtensionLauncherContextMenu::GetMenuItems(item);
  // ash::MenuItemList items;
  // // TODO(msw): Support these extension context items. 
  // // extension_context_items_ = base::MakeUnique<extensions::ContextMenuMatcher>(
  // //     controller->profile(), this, this,
  // //     base::Bind(MenuItemHasLauncherContext));
  // const ash::ShelfItem* shelf_item = controller->GetItem(shelf_id());
  // LOG(ERROR) << "MSW ExtensionAppWindowLauncherItemController::GetContextMenuItems A " << shelf_item->type;
  // if (shelf_item->type == ash::TYPE_PINNED_APP ||
  //     shelf_item->type == ash::TYPE_APP) {
  //   LOG(ERROR) << "MSW ExtensionAppWindowLauncherItemController::GetContextMenuItems B";
  //   // V1 apps can be started from the menu - but V2 apps should not.
  //   if (!controller->IsPlatformApp(shelf_id())) {
  //     LOG(ERROR) << "MSW ExtensionAppWindowLauncherItemController::GetContextMenuItems C";
  //     ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
  //     item->command_id = ash::ShelfContextMenuItem::MENU_OPEN_NEW;
  //     switch (GetLaunchType(shelf_id())) {
  //       case extensions::LAUNCH_TYPE_PINNED:
  //       case extensions::LAUNCH_TYPE_REGULAR:
  //         item->label = l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_TAB);
  //       case extensions::LAUNCH_TYPE_FULLSCREEN:
  //       case extensions::LAUNCH_TYPE_WINDOW:
  //         item->label = l10n_util::GetStringUTF16(IDS_APP_LIST_CONTEXT_MENU_NEW_WINDOW);
  //       default:
  //         NOTREACHED();
  //     }
  //     item->enabled = true;
  //     items.push_back(std::move(item));

  //     ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
  //     separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
  //     items.push_back(std::move(separator));
  //   }

  //   AddPinContextMenuItem(&items, shelf_id());

  //   if (controller->IsOpen(shelf_id())) {
  //     ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
  //     close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
  //     close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  //     close->enabled = true;
  //     items.push_back(std::move(close));
  //   }

  //   if (!controller->IsPlatformApp(shelf_id()) &&
  //       shelf_item->type == ash::TYPE_PINNED_APP) {
  //     ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
  //     separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
  //     items.push_back(std::move(separator));

  //     const int launch_type = GetLaunchType(shelf_id());
  //     if (extensions::util::IsNewBookmarkAppsEnabled()) {
  //       // With bookmark apps enabled, hosted apps launch in a window by
  //       // default. This menu item is re-interpreted as a single, toggle-able
  //       // option to launch the hosted app as a tab.
  //       ash::mojom::MenuItemPtr item(ash::mojom::MenuItem::New());
  //       item->type = ash::mojom::MenuItem::Type::CHECK;
  //       item->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_WINDOW;
  //       item->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
  //       item->enabled = true;
  //       item->checked = launch_type == extensions::LAUNCH_TYPE_WINDOW;
  //       items.push_back(std::move(item));
  //     } else {
  //       ash::mojom::MenuItemPtr regular(ash::mojom::MenuItem::New());
  //       regular->type = ash::mojom::MenuItem::Type::CHECK;
  //       regular->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_REGULAR_TAB;
  //       regular->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_REGULAR);
  //       regular->enabled = true;
  //       regular->checked = launch_type == extensions::LAUNCH_TYPE_REGULAR;
  //       items.push_back(std::move(regular));

  //       ash::mojom::MenuItemPtr pinned(ash::mojom::MenuItem::New());
  //       pinned->type = ash::mojom::MenuItem::Type::CHECK;
  //       pinned->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_PINNED_TAB;
  //       pinned->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_PINNED);
  //       pinned->enabled = true;
  //       pinned->checked = launch_type == extensions::LAUNCH_TYPE_PINNED;
  //       items.push_back(std::move(pinned));

  //       ash::mojom::MenuItemPtr window(ash::mojom::MenuItem::New());
  //       window->type = ash::mojom::MenuItem::Type::CHECK;
  //       window->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_WINDOW;
  //       window->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_WINDOW);
  //       window->enabled = true;
  //       window->checked = launch_type == extensions::LAUNCH_TYPE_WINDOW;
  //       items.push_back(std::move(window));

  //       // Even though the launch type is Full Screen it is more accurately
  //       // described as Maximized in Ash.
  //       ash::mojom::MenuItemPtr fullscreen(ash::mojom::MenuItem::New());
  //       fullscreen->type = ash::mojom::MenuItem::Type::CHECK;
  //       fullscreen->command_id = ash::ShelfContextMenuItem::LAUNCH_TYPE_FULLSCREEN;
  //       fullscreen->label = l10n_util::GetStringUTF16(IDS_APP_CONTEXT_MENU_OPEN_MAXIMIZED);
  //       fullscreen->enabled = true;
  //       fullscreen->checked = launch_type == extensions::LAUNCH_TYPE_FULLSCREEN;
  //       items.push_back(std::move(fullscreen));
  //     }
  //   }
  // } else if (shelf_item->type == ash::TYPE_BROWSER_SHORTCUT) {
  //   ash::mojom::MenuItemPtr window(ash::mojom::MenuItem::New());
  //   window->command_id = ash::ShelfContextMenuItem::MENU_NEW_WINDOW;
  //   window->label = l10n_util::GetStringUTF16(IDS_APP_LIST_NEW_WINDOW);
  //   window->enabled = true;
  //   items.push_back(std::move(window));

  //   if (!controller->profile()->IsGuestSession()) {
  //     ash::mojom::MenuItemPtr incognito(ash::mojom::MenuItem::New());
  //     incognito->command_id = ash::ShelfContextMenuItem::MENU_NEW_INCOGNITO_WINDOW;
  //     incognito->label = l10n_util::GetStringUTF16(IDS_APP_LIST_NEW_INCOGNITO_WINDOW);
  //     incognito->enabled = true;
  //     items.push_back(std::move(incognito));
  //   }
  //   if (!BrowserShortcutLauncherItemController(controller->shelf_model())
  //            .IsListOfActiveBrowserEmpty()) {
  //     ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
  //     close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
  //     close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  //     close->enabled = true;
  //     items.push_back(std::move(close));
  //   }
  // } else if (shelf_item->type == ash::TYPE_DIALOG ||
  //            controller->IsOpen(shelf_item->id)) {
  //   ash::mojom::MenuItemPtr close(ash::mojom::MenuItem::New());
  //   close->command_id = ash::ShelfContextMenuItem::MENU_CLOSE;
  //   close->label = l10n_util::GetStringUTF16(IDS_LAUNCHER_CONTEXT_MENU_CLOSE);
  //   close->enabled = true;
  //   items.push_back(std::move(close));
  // }
  // ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
  // separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
  // items.push_back(std::move(separator));

  // if (shelf_item->type == ash::TYPE_PINNED_APP ||
  //     shelf_item->type == ash::TYPE_APP) {
  //   const extensions::MenuItem::ExtensionKey app_key(shelf_item->id.app_id);
  //   if (!app_key.empty()) {
  //     // TODO(msw): Support these extension context items. 
  //     // int index = 0;
  //     // extension_items_->AppendExtensionItems(app_key, base::string16(), &index,
  //     //                                        false);  // is_action_menu
  //     // ash::mojom::MenuItemPtr separator(ash::mojom::MenuItem::New());
  //     // separator->type = ash::mojom::MenuItem::Type::SEPARATOR;
  //     // items.push_back(std::move(separator));
  //   }
  // }
  // // TODO(msw): Ash will add the shelf context menu items. 
  // // AddShelfContextMenuItems(&items, display_id); 
  // return items;
}

void ExtensionAppWindowLauncherItemController::ExecuteCommand(
    bool from_context_menu,
    uint32_t command_id,
    int32_t event_flags,
    int64_t display_id) {
  ChromeLauncherController::instance()->ActivateShellApp(app_id(), command_id);
}

void ExtensionAppWindowLauncherItemController::OnWindowTitleChanged(
    aura::Window* window) {
  ui::BaseWindow* base_window = GetAppWindow(window);
  extensions::AppWindowRegistry* app_window_registry =
      extensions::AppWindowRegistry::Get(
          ChromeLauncherController::instance()->profile());
  extensions::AppWindow* app_window =
      app_window_registry->GetAppWindowForNativeWindow(
          base_window->GetNativeWindow());

  // Use the window title (if set) to differentiate show_in_shelf window shelf
  // items instead of the default behavior of using the app name.
  if (app_window->show_in_shelf()) {
    base::string16 title = window->GetTitle();
    if (!title.empty())
      ChromeLauncherController::instance()->SetItemTitle(shelf_id(), title);
  }
}
