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

// constexpr int kFixedMenuWidth = 240;
constexpr int kAvatarIconSize = 16;

}  // namespace

// static
void DiceAccountsMenu::Show(const std::vector<AccountInfo>& accounts,
                            const std::vector<gfx::Image>& icons,
                            views::View* anchor_view) {
  DCHECK(accounts.size() == icons.size());
  gfx::Image default_icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          profiles::GetPlaceholderAvatarIconResourceID());

  // Build the menu.
  ui::SimpleMenuModel* menu =
      new ui::SimpleMenuModel(new DiceAccountsMenu(accounts));
  for (size_t idx = 0; idx < accounts.size(); idx++) {
    menu->AddItem(idx, base::UTF8ToUTF16(accounts[idx].email));
    gfx::Image circle_icon = profiles::GetSizedAvatarIcon(
        icons[idx].IsEmpty() ? default_icon : icons[idx], true, kAvatarIconSize,
        kAvatarIconSize, profiles::SHAPE_CIRCLE);
    menu->SetIcon(idx, circle_icon);
  }
  // TODO add string to resources, create default_circle_icon above
  menu->AddItem(accounts.size(), base::ASCIIToUTF16("Use another account"));
  gfx::Image circle_icon =
      profiles::GetSizedAvatarIcon(default_icon, true, kAvatarIconSize,
                                   kAvatarIconSize, profiles::SHAPE_CIRCLE);
  menu->SetIcon(accounts.size(), circle_icon);
  // Open the menu below the anchor.
  gfx::Point anchor_loc;
  views::View::ConvertPointToScreen(anchor_view, &anchor_loc);
  views::MenuRunner* runner =
      new views::MenuRunner(menu, views::MenuRunner::CONTEXT_MENU);
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

void DiceAccountsMenu::ExecuteCommand(int idx, int event_flags) {
  LOG(ERROR) << "Account " << accounts_[idx].email << " was clicked";
}
