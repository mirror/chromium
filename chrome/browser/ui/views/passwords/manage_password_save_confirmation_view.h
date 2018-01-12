// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORD_SAVE_CONFIRMATION_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORD_SAVE_CONFIRMATION_VIEW_H_

#include "chrome/browser/ui/views/passwords/manage_passwords_bubble_delegate_view_base.h"
#include "ui/views/view.h"

// A view confirming to the user that a password was saved and offering a link
// to the Google account manager.
class ManagePasswordSaveConfirmationView
    : public ManagePasswordsBubbleDelegateViewBase {
 public:
  explicit ManagePasswordSaveConfirmationView(
      content::WebContents* web_contents,
      views::View* anchor_view,
      const gfx::Point& anchor_point,
      DisplayReason reason);
  ~ManagePasswordSaveConfirmationView() override;

 private:
  // LocationBarBubbleDelegateView:
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool ShouldShowCloseButton() const override;
  bool Accept() override;
  bool Close() override;
  gfx::Size CalculatePreferredSize() const override;

  DISALLOW_COPY_AND_ASSIGN(ManagePasswordSaveConfirmationView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORD_SAVE_CONFIRMATION_VIEW_H_
