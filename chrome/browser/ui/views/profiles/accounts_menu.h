// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PROFILES_ACCOUNTS_MENU_H_
#define CHROME_BROWSER_UI_VIEWS_PROFILES_ACCOUNTS_MENU_H_

#include <vector>

#include "components/signin/core/browser/account_info.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace views {
class ButtonListener;
class View;
}  // namespace views

class AccountsMenu : public views::BubbleDialogDelegateView {
 public:
  static void ShowBubble(const std::vector<AccountInfo>& accounts,
                         views::ButtonListener* listener,
                         views::View* anchor_view);

 private:
  AccountsMenu(const std::vector<AccountInfo>& accounts,
               views::ButtonListener* listener,
               views::View* anchor_view);
  ~AccountsMenu() override;

  // views::BubbleDialogDelegateView:
  void Init() override;
  int GetDialogButtons() const override;
  void WindowClosing() override;
  void OnWidgetDestroyed(views::Widget* widget) override;

  views::ButtonListener* listener_;
  std::vector<AccountInfo> accounts_;

  DISALLOW_COPY_AND_ASSIGN(AccountsMenu);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_ACCOUNTS_MENU_H_
