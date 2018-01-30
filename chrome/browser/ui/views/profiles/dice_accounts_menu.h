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

class DiceAccountsMenu : public ui::SimpleMenuModel::Delegate {
 public:
  static void Show(const std::vector<AccountInfo>& accounts,
                   const std::vector<gfx::Image>& icons,
                   views::View* anchor_view);

  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  DiceAccountsMenu(const std::vector<AccountInfo>& accounts);
  ~DiceAccountsMenu() override;

  std::vector<AccountInfo> accounts_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenu);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_
