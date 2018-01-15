// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/sync/dice_bubble_sync_promo_view.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_ui_util.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/sync/dice_signin_button.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

DiceBubbleSyncPromoView::DiceBubbleSyncPromoView(
    Profile* profile,
    DiceBubbleSyncPromoViewDelegate* delegate,
    int no_accounts_title_resource_id,
    int accounts_title_resource_id)
    : views::View(), delegate_(delegate) {
  std::unique_ptr<views::BoxLayout> layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(),
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_RELATED_CONTROL_VERTICAL));
  SetLayoutManager(std::move(layout));

  std::vector<AccountInfo> accounts =
      signin_ui_util::GetAccountsForDicePromos(profile);
  int title_resource_id = accounts.empty() ? no_accounts_title_resource_id
                                           : accounts_title_resource_id;

  // Add the title label.
  base::string16 title_text = l10n_util::GetStringUTF16(title_resource_id);
  views::Label* title = new views::Label(title_text);
  AddChildView(title);

  // Add the sign-in button.
  signin_button_ = accounts.empty() ? new DiceSigninButton(this)
                                    : new DiceSigninButton(accounts[0], this);
  AddChildView(signin_button_);
}

DiceBubbleSyncPromoView::~DiceBubbleSyncPromoView() {}

void DiceBubbleSyncPromoView::ButtonPressed(views::Button* sender,
                                            const ui::Event& event) {
  if (sender == signin_button_) {
    VLOG(1) << "Sign In button pressed";
    if (signin_button_->account())
      delegate_->EnableSync(signin_button_->account().value());
    else
      delegate_->ShowSignin();
    return;
  }

  if (sender == signin_button_->drop_down_arrow()) {
    VLOG(1) << "Drop down arrow pressed";
    // TODO(msarda): Show the other accounts menu.
    return;
  }

  NOTREACHED();
}
