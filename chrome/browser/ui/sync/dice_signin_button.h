// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SYNC_DICE_SIGNIN_BUTTON_H_
#define CHROME_BROWSER_UI_SYNC_DICE_SIGNIN_BUTTON_H_

#include "components/signin/core/browser/account_info.h"
#include "ui/views/controls/button/md_text_button.h"

// Sign-in button used for Dice that presents a button containing the account
// information (avatar image and email) that invites the user to sign in to
// chrome.
//
// The button also presents on the right hand side a drown-down arrow button
// that the user can interact with.
class DiceSigninButton : public views::MdTextButton {
 public:
  // |signin_button_istener| will be called for events on |this|
  // |drop_down_arrow_listener| will be called for events originating from
  // the drop down arrow button.
  explicit DiceSigninButton(const AccountInfo& account_info,
                            views::ButtonListener* signin_button_istener,
                            views::ButtonListener* drop_down_arrow_listener);
  ~DiceSigninButton() override;

  views::LabelButton* drop_down_arrow() const { return arrow_; }

  // views::MdTextButton:
  gfx::Rect GetChildAreaBounds() override;
  void Layout() override;

 private:
  views::LabelButtonLabel* subtitle_;
  views::View* divider_;
  views::LabelButton* arrow_;
  DISALLOW_COPY_AND_ASSIGN(DiceSigninButton);
};

#endif  // CHROME_BROWSER_UI_SYNC_DICE_SIGNIN_BUTTON_H_
