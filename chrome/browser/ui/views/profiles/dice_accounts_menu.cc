// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/dice_accounts_menu.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view.h"

namespace {

constexpr int kAvatarIconSize = 16;

gfx::Image SizeAndCircleIcon(const gfx::Image& icon) {
  return profiles::GetSizedAvatarIcon(icon, true, kAvatarIconSize,
                                      kAvatarIconSize, profiles::SHAPE_CIRCLE);
}

ui::SimpleMenuModel* menu = nullptr;

}  // namespace

// static
void DiceAccountsMenu::Show(const std::vector<AccountInfo>& accounts,
                            const std::vector<gfx::Image>& icons,
                            views::View* anchor_view) {
  // If menu is already showing, do nothing.
  if (menu)
    return;
  DCHECK_EQ(accounts.size(), icons.size());
  gfx::Image default_icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          profiles::GetPlaceholderAvatarIconResourceID());
  // Build the menu.
  menu = new ui::SimpleMenuModel(new DiceAccountsMenu(accounts));
  menu->AddSeparator(ui::SPACING_SEPARATOR);
  for (size_t idx = 0; idx < accounts.size(); idx++) {
    menu->AddItem(idx, base::UTF8ToUTF16(accounts[idx].email));
    menu->SetIcon(
        menu->GetIndexOfCommandId(idx),
        SizeAndCircleIcon(icons[idx].IsEmpty() ? default_icon : icons[idx]));
    menu->AddSeparator(ui::SPACING_SEPARATOR);
  }
  // Add the "Use another account" button at the end.
  menu->AddItem(accounts.size(), base::ASCIIToUTF16("Use another account"));
  menu->SetIcon(menu->GetIndexOfCommandId(accounts.size()),
                SizeAndCircleIcon(default_icon));
  menu->AddSeparator(ui::SPACING_SEPARATOR);
  // Open the menu.
  gfx::Point anchor_loc;
  views::View::ConvertPointToScreen(anchor_view, &anchor_loc);
  views::MenuRunner* runner =
      new views::MenuRunner(menu, views::MenuRunner::COMBOBOX);
  runner->RunMenuAt(anchor_view->GetWidget(), nullptr,
                    gfx::Rect(anchor_loc, anchor_view->size()),
                    views::MENU_ANCHOR_BUBBLE_BELOW, ui::MENU_SOURCE_MOUSE);
}

DiceAccountsMenu::DiceAccountsMenu(const std::vector<AccountInfo>& accounts)
    : accounts_(accounts) {}

DiceAccountsMenu::~DiceAccountsMenu() {}

bool DiceAccountsMenu::IsCommandIdChecked(int command_id) const {
  return false;
}

bool DiceAccountsMenu::IsCommandIdEnabled(int command_id) const {
  return true;
}

void DiceAccountsMenu::ExecuteCommand(int id, int event_flags) {
  NOTIMPLEMENTED() << (id < int(accounts_.size()) ? accounts_[id].email
                                                  : "\"Use another account\"")
                   << " was clicked";
}

void DiceAccountsMenu::MenuClosed(ui::SimpleMenuModel* source) {
  if (source == menu)
    menu = nullptr;
}
