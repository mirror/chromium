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

class ScopedCloseBubbleOnDeactivate;

// This bubble view displays a list of selectable accounts given by |accounts|.
// It is used to select an account for turning on Sync when DICE is enabled.
// TODO(tangltom): Add action handling.
class DiceAccountsMenu : public views::BubbleDialogDelegateView {
 public:
  // Shows a list of selectable accounts(given by |accounts|) in a bubble, if it
  // is not already showing. When the bubble window loses focus it will close
  // itself. Additionally, if the window associated to |parent_bubble| loses
  // focus, e.g. when the user clicks outside, the parent bubble is closed as
  // well. The position of the bubble is set according to |anchor_view|.
  static void ShowBubble(views::BubbleDialogDelegateView* parent_bubble,
                         const std::vector<AccountInfo>& accounts,
                         views::View* anchor_view);

 private:
  friend class DiceAccountsMenuTestProxy;

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

  // Saves the |close_on_deactivate| state of the parent bubble.
  std::unique_ptr<ScopedCloseBubbleOnDeactivate> parent_bubble_saver_;

  DISALLOW_COPY_AND_ASSIGN(DiceAccountsMenu);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_DICE_ACCOUNTS_MENU_H_
