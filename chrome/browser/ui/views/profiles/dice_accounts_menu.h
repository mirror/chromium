// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_
#define CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_

#include <vector>

#include "components/signin/core/browser/account_info.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace views {
class View;
}  // namespace views

// This bubble view displays a list of selectable accounts given by |accounts|.
// It is used to select an account for turning on Sync when DICE is enabled.
// TODO(tangltom): Add action handling.
class DiceAccountsMenu : public views::BubbleDialogDelegateView {
 public:
  // Shows the bubble if it is not already showing. When the bubble window loses
  // focus it will close itself. If the window of |anchor_view| loses focus too,
  // e.g. when the user clicks outside, the bubble will close the |anchor_view|
  // bubble too. The position is set according to |anchor_view|.
  static views::Widget* ShowBubble(
      views::BubbleDialogDelegateView* parent_bubble,
      const std::vector<AccountInfo>& accounts,
      views::View* anchor_view);

 private:
  DiceAccountsMenu(views::BubbleDialogDelegateView* parent_bubble,
                   const std::vector<AccountInfo>& accounts,
                   views::View* anchor_view);
  ~DiceAccountsMenu() override;

  // views::BubbleDialogDelegateView:
  void Init() override;
  int GetDialogButtons() const override;
  void WindowClosing() override;
  void OnWidgetDestroyed(views::Widget* widget) override;

  views::BubbleDialogDelegateView* parent_bubble_;
  std::vector<AccountInfo> accounts_;

  // Pointer to an DiceAccountMenu object to make sure there is only one at a
  // time.
  static DiceAccountsMenu* menu_bubble_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenu);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_
