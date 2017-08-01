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

void ExtensionAppWindowLauncherItemController::ExecuteCommandImpl(
    bool from_context_menu,
    int64_t command_id,
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
