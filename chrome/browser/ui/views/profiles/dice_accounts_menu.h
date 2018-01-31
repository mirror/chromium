// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_
#define CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_

#include <vector>

#include "components/signin/core/browser/account_info.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/image/image.h"

namespace views {
class View;
}  // namespace views

// Accounts menu class used to select an account for turning on Sync when DICE
// is enabled.
// TODO(tangltom): Add action handling.
class DiceAccountsMenu : public ui::SimpleMenuModel::Delegate {
 public:
  // Shows a list of selectable accounts(given by |accounts|) and a "Use another
  // accounts" option at the bottom of a submenu. The list |icons| corresponds
  // to the account icons of |accounts|. The menu is placed on the bottom of
  // |anchor_view|. If the dice accounts menu is already showing, this function
  // does nothing.
  static void Show(const std::vector<AccountInfo>& accounts,
                   const std::vector<gfx::Image>& icons,
                   views::View* anchor_view);

 private:
  explicit DiceAccountsMenu(const std::vector<AccountInfo>& accounts);
  ~DiceAccountsMenu() override;

  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void MenuClosed(ui::SimpleMenuModel* source) override;

  std::vector<AccountInfo> accounts_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenu);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_
