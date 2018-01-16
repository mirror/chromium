// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SYNC_DICE_BUBBLE_SYNC_PROMO_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_SYNC_DICE_BUBBLE_SYNC_PROMO_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/sync/bubble_sync_promo_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

class Profile;
class DiceSigninButton;

class DiceBubbleSyncPromoView : public views::View,
                                public views::ButtonListener {
 public:
  // |delegate| is not owned by DiceBubbleSyncPromoView.
  DiceBubbleSyncPromoView(Profile* profile,
                          BubbleSyncPromoDelegate* delegate,
                          int no_accounts_title_resource_id,
                          int accounts_title_resource_id);
  ~DiceBubbleSyncPromoView() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  // views::StyledLabel:
  const char* GetClassName() const override;

  // Delegate, to handle clicks on the sign-in buttons.
  BubbleSyncPromoDelegate* delegate_;

  DiceSigninButton* signin_button_;

  DISALLOW_COPY_AND_ASSIGN(DiceBubbleSyncPromoView);
};
#endif  // CHROME_BROWSER_UI_VIEWS_SYNC_DICE_BUBBLE_SYNC_PROMO_VIEW_H_
