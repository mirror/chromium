// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/safe_browsing/password_reuse_modal_warning_dialog.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/vector_icons/vector_icons.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace safe_browsing {

namespace {
// Size of warning icon in top left of dialog.
constexpr int kIconSize = 24;
constexpr int kTextWidth = 450;

}  // namespace

PasswordReuseModalWarningDialog::PasswordReuseModalWarningDialog(
    content::WebContents* web_contents,
    const OnWarningDone& done)
    : TabModalConfirmDialogDelegate(web_contents),
      show_softer_warning_(
          PasswordProtectionService::ShouldShowSofterWarning()),
      done_(done) {
  DCHECK(!done_.is_null());
  Init();
}

PasswordReuseModalWarningDialog::~PasswordReuseModalWarningDialog() {}

void PasswordReuseModalWarningDialog::Init() {
  icon_ = base::MakeUnique<gfx::Image>(CreateIcon()),
  contents_view_ = base::MakeUnique<views::View>();
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const gfx::Insets content_insets =
      provider->GetInsetsMetric(views::INSETS_DIALOG_CONTENTS);
  views::GridLayout* layout = new views::GridLayout(contents_view_.get());
  contents_view_->SetLayoutManager(layout);
  contents_view_->SetBorder(views::CreateEmptyBorder(
      0, content_insets.left(), content_insets.bottom(), 0));
  AddChildView(contents_view_.get());

  // Warning icon.
  const gfx::ImageSkia* image = icon_->ToImageSkia();
  gfx::Size size(image->width(), image->height());
  if (size.width() > kIconSize || size.height() > kIconSize)
    size = gfx::Size(kIconSize, kIconSize);
  views::ImageView* icon = new views::ImageView();
  icon->SetImageSize(size);
  icon->SetImage(*image);
  // Warning title.
  views::Label* title =
      new views::Label(GetTitle(), views::style::CONTEXT_DIALOG_TITLE);
  if (show_softer_warning_)
    title->SetEnabledColor(gfx::kChromeIconGrey);
  else
    title->SetEnabledColor(gfx::kGoogleRed700);
  title->SetMultiLine(true);
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SizeToFit(title->GetPreferredSize().width());
  // Warning detail.
  views::Label* detail = new views::Label(GetDialogMessage());
  detail->SetMultiLine(true);
  detail->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail->SizeToFit(kTextWidth);

  int padding_size =
      provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_HORIZONTAL);
  int preferred_content_width = content_insets.left() + kIconSize +
                                padding_size + kTextWidth +
                                content_insets.right();
  // Configure layout
  layout->AddPaddingRow(0, padding_size * 2);
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0,  // no resizing
                        views::GridLayout::USE_PREF,
                        0,  // no fixed width
                        kIconSize);
  column_set->AddPaddingColumn(0, padding_size);

  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0,  // no resizing
                        views::GridLayout::USE_PREF,
                        0,  // no fixed width
                        kTextWidth);
  // column_set->AddPaddingColumn(0, content_insets.right());
  layout->StartRow(0, 0);
  layout->AddView(icon, 1, 1);
  layout->AddView(title, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::LEADING);
  layout->StartRow(1, 0);
  layout->SkipColumns(1);
  layout->AddView(detail, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::LEADING);
  SetPreferredSize(gfx::Size(preferred_content_width,
                             contents_view_->GetPreferredSize().height()));
}

base::string16 PasswordReuseModalWarningDialog::GetTitle() {
  return show_softer_warning_
             ? l10n_util::GetStringUTF16(
                   IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY_SOFTER)
             : l10n_util::GetStringUTF16(IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY);
}

base::string16 PasswordReuseModalWarningDialog::GetDialogMessage() {
  return l10n_util::GetStringUTF16(IDS_PAGE_INFO_CHANGE_PASSWORD_DETAILS);
}

gfx::Image* PasswordReuseModalWarningDialog::GetIcon() {
  return icon_.get();
}

bool PasswordReuseModalWarningDialog::Accept() {
  done_.Run(PasswordProtectionService::MODAL_DIALOG,
            PasswordProtectionService::CHANGE_PASSWORD);
  return true;
}

bool PasswordReuseModalWarningDialog::Cancel() {
  done_.Run(PasswordProtectionService::MODAL_DIALOG,
            PasswordProtectionService::IGNORE_WARNING);
  return true;
}

// Navigate away or hit "ESC" key.
bool PasswordReuseModalWarningDialog::Close() {
  done_.Run(PasswordProtectionService::MODAL_DIALOG,
            PasswordProtectionService::CLOSE);
  return true;
}

gfx::ImageSkia PasswordReuseModalWarningDialog::CreateIcon() {
  return show_softer_warning_
             ? gfx::CreateVectorIcon(kSecurityAlertIcon, gfx::kChromeIconGrey)
             : gfx::CreateVectorIcon(vector_icons::kWarningIcon,
                                     gfx::kGoogleRed700);
}

bool PasswordReuseModalWarningDialog::ShouldShowCloseButton() const {
  return false;
}

ui::ModalType PasswordReuseModalWarningDialog::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

void PasswordReuseModalWarningDialog::Layout() {
  contents_view_->SetBounds(0, 0, width(), height());
  DialogDelegateView::Layout();
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

}  // namespace safe_browsing
