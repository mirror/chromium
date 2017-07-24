// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_CHROME_LAUNCHER_CONTROLLER_UTIL_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_CHROME_LAUNCHER_CONTROLLER_UTIL_H_

#include "ash/public/cpp/shelf_item_delegate.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"

namespace extensions {
class Extension;
}

class Browser;

// Returns true if |browser| is owned by the active user.
bool IsBrowserFromActiveUser(Browser* browser);

// Returns the extension identified by |app_id|.
const extensions::Extension* GetExtensionForAppID(const std::string& app_id,
                                                  Profile* profile);

// Returns whether the app can be pinned, and whether the pinned app are
// editable or fixed
AppListControllerDelegate::Pinnable GetPinnableForAppID(
    const std::string& app_id,
    Profile* profile);

// Add the context menu item to pin or unpin the item with the given |id|.
void AddPinContextMenuItem(ash::MenuItemList* menu, const ash::ShelfID& id);

// Add the context menu items to change shelf auto-hide and alignment settings
// and to change the wallpaper for the display with the given |display_id|.
void AddShelfContextMenuItems(ash::MenuItemList* menu, int64_t display_id);

// Executes a context menu command for the given shelf item and display.
void ExecuteContextMenuCommand(const ash::ShelfID& id,
                               uint32_t command_id,
                               int64_t display_id);

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_CHROME_LAUNCHER_CONTROLLER_UTIL_H_
