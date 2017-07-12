// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_PROTECTION_WARNING_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_PROTECTION_WARNING_DIALOG_H_

#include "base/callback.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog_delegate.h"
#include "components/safe_browsing/password_protection/password_protection_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace safe_browsing {

// Implementation of password reuse modal dialog. We use this class rather than
// directly using TabMoDalConfirmDialog so that we can use custom formatting on
// the dialog title and body.
class PasswordProtectionWarningDialog : public TabModalConfirmDialogDelegate,
                                        public views::DialogDelegateView {
 public:
  using OnDone =
      base::Callback<void(PasswordProtectionService::WarningDialogAction)>;
  PasswordProtectionWarningDialog(content::WebContents* web_contents,
                                  const OnDone& done);

  ~PasswordProtectionWarningDialog() override;

  void Init();

  // TabModalConfirmDialogDelegate overrides.
  base::string16 GetTitle() override;
  base::string16 GetDialogMessage() override;
  bool Accept() override;
  bool Cancel() override;
  bool Close() override;

 private:
  // views::WidgetDelegate overrides.
  bool ShouldShowCloseButton() const override;
  ui::ModalType GetModalType() const override;
  void Layout() override;

  // ui::DialogModel: overrides.
  int GetDefaultDialogButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;

  bool softer_warning_;
  std::unique_ptr<gfx::Image> icon_;
  std::unique_ptr<views::View> contents_view_;
  OnDone done_;
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_PROTECTION_WARNING_DIALOG_H_
