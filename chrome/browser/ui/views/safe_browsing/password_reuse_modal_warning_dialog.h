// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_REUSE_MODAL_WARNING_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_REUSE_MODAL_WARNING_DIALOG_H_

#include "base/callback.h"
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog_delegate.h"
#include "components/safe_browsing/password_protection/password_protection_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace safe_browsing {

// Implementation of password reuse modal dialog. We use this class rather than
// directly using TabModalConfirmDialog so that we can use custom formatting on
// dialog.
class PasswordReuseModalWarningDialog : public TabModalConfirmDialogDelegate,
                                        public views::DialogDelegateView {
 public:
  PasswordReuseModalWarningDialog(content::WebContents* web_contents,
                                  OnWarningDone done_callback);

  // TabModalConfirmDialogDelegate overrides.
  base::string16 GetTitle() override;
  base::string16 GetDialogMessage() override;
  bool Accept() override;
  bool Cancel() override;
  bool Close() override;

 private:
  friend class PasswordReuseModalWarningTest;

  ~PasswordReuseModalWarningDialog() override;

  // views::DialogDelegateView:
  bool ShouldShowCloseButton() const override;
  ui::ModalType GetModalType() const override;
  int GetDefaultDialogButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  gfx::ImageSkia GetWindowIcon() override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowWindowIcon() const override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  views::View* GetContentsView() override;
  void DeleteDelegate() override;

  const bool show_softer_warning_;
  bool button_clicked_;

  views::View* dialog_message_view_;
  OnWarningDone done_callback_;
  DISALLOW_COPY_AND_ASSIGN(PasswordReuseModalWarningDialog);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_REUSE_MODAL_WARNING_DIALOG_H_
