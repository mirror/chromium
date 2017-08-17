// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/safe_browsing/password_reuse_modal_warning_dialog.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"

namespace safe_browsing {

namespace {
// Size of warning icon in top left of dialog.
constexpr int kIconSize = 24;
// Width of warning dialog message.
constexpr int kTextWidth = 450;

}  // namespace

void ShowPasswordReuseModalWarningDialog(content::WebContents* web_contents,
                                         OnWarningDone done_callback) {
  PasswordReuseModalWarningDialog* dialog = new PasswordReuseModalWarningDialog(
      web_contents, std::move(done_callback));
  constrained_window::ShowWebModalDialogViews(dialog, web_contents);
}

PasswordReuseModalWarningDialog::PasswordReuseModalWarningDialog(
    content::WebContents* web_contents,
    OnWarningDone done_callback)
    : TabModalConfirmDialogDelegate(web_contents),
      show_softer_warning_(
          PasswordProtectionService::ShouldShowSofterWarning()),
      button_clicked_(false),
      dialog_message_view_(nullptr),
      done_callback_(std::move(done_callback)) {
  dialog_message_view_ = new views::View;
  views::GridLayout* layout =
      views::GridLayout::CreatePanel(dialog_message_view_);
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                        views::GridLayout::FIXED, kIconSize, 0);
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  int padding_size =
      provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_HORIZONTAL);
  column_set->AddPaddingColumn(0, padding_size);

  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                        views::GridLayout::FIXED, kTextWidth, 0);

  views::Label* message_body_label = new views::Label(GetDialogMessage());
  message_body_label->SetMultiLine(true);
  message_body_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  message_body_label->SetAllowCharacterBreak(true);
  message_body_label->SizeToFit(kTextWidth);

  layout->StartRow(0, 0);
  layout->SkipColumns(1);
  layout->AddView(message_body_label);
}

base::string16 PasswordReuseModalWarningDialog::GetTitle() {
  return GetWindowTitle();
}

base::string16 PasswordReuseModalWarningDialog::GetDialogMessage() {
  return l10n_util::GetStringUTF16(IDS_PAGE_INFO_CHANGE_PASSWORD_DETAILS);
}

bool PasswordReuseModalWarningDialog::Accept() {
  std::move(done_callback_).Run(PasswordProtectionService::CHANGE_PASSWORD);
  button_clicked_ = true;
  return true;
}

bool PasswordReuseModalWarningDialog::Cancel() {
  std::move(done_callback_).Run(PasswordProtectionService::IGNORE_WARNING);
  button_clicked_ = true;
  return true;
}

bool PasswordReuseModalWarningDialog::Close() {
  // If user didn't click on any button and Close() is called, it is
  // probably because user hit 'ESC', navigated away or closed this tab.
  if (!button_clicked_)
    std::move(done_callback_).Run(PasswordProtectionService::CLOSE);
  return true;
}

PasswordReuseModalWarningDialog::~PasswordReuseModalWarningDialog() {}

bool PasswordReuseModalWarningDialog::ShouldShowCloseButton() const {
  return false;
}

ui::ModalType PasswordReuseModalWarningDialog::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

int PasswordReuseModalWarningDialog::GetDefaultDialogButton() const {
  return ui::DIALOG_BUTTON_OK;
}

base::string16 PasswordReuseModalWarningDialog::GetDialogButtonLabel(
    ui::DialogButton button) const {
  switch (button) {
    case ui::DIALOG_BUTTON_OK:
      return l10n_util::GetStringUTF16(IDS_PAGE_INFO_CHANGE_PASSWORD_BUTTON);
    case ui::DIALOG_BUTTON_CANCEL:
      return l10n_util::GetStringUTF16(
          IDS_PAGE_INFO_IGNORE_PASSWORD_WARNING_BUTTON);
    default:
      return DialogDelegate::GetDialogButtonLabel(button);
  }
}

gfx::ImageSkia PasswordReuseModalWarningDialog::GetWindowIcon() {
  return show_softer_warning_
             ? gfx::CreateVectorIcon(kSecurityIcon, kIconSize,
                                     gfx::kChromeIconGrey)
             : gfx::CreateVectorIcon(vector_icons::kWarningIcon, kIconSize,
                                     gfx::kGoogleRed700);
}

base::string16 PasswordReuseModalWarningDialog::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(
      show_softer_warning_ ? IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY_SOFTER
                           : IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY);
}

bool PasswordReuseModalWarningDialog::ShouldShowWindowIcon() const {
  return true;
}

}  // namespace safe_browsing
