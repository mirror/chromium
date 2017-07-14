// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/safe_browsing/password_protection_warning_dialog.h"

#include "base/metrics/field_trial_params.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/vector_icons/vector_icons.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace safe_browsing {

namespace {
// Size of warning icon in top left of dialog.
constexpr int kIconSize = 24;

}  // namespace

PasswordProtectionWarningDialog::PasswordProtectionWarningDialog(
    content::WebContents* web_contents,
    const OnDone& done)
    : TabModalConfirmDialogDelegate(web_contents),
      softer_warning_(base::GetFieldTrialParamByFeatureAsBool(
          safe_browsing::kGoogleBrandedPhishingWarning,
          "softer_warning",
          false)),
      done_(done) {
  DCHECK(!done_.is_null());
  Init();
}

PasswordProtectionWarningDialog::~PasswordProtectionWarningDialog() {}

void PasswordProtectionWarningDialog::Init() {
  icon_ = base::MakeUnique<gfx::Image>(
      gfx::CreateVectorIcon(ui::kWarningIcon, gfx::kGoogleRed700));
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
  title->SetEnabledColor(gfx::kGoogleRed700);
  title->SetMultiLine(true);
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SizeToFit(title->GetPreferredSize().width());
  // Warning detail.
  int preferred_content_width =
      content_insets.left() + kIconSize +
      provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_HORIZONTAL) +
      title->GetPreferredSize().width() + content_insets.right();
  views::Label* detail = new views::Label(GetDialogMessage());
  detail->SetMultiLine(true);
  detail->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  detail->SizeToFit(preferred_content_width);
  // Configure layout
  layout->AddPaddingRow(0, 10);
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0,  // no resizing
                        views::GridLayout::USE_PREF,
                        0,  // no fixed width
                        kIconSize);
  column_set->AddPaddingColumn(
      0, provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_HORIZONTAL));

  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::LEADING,
                        0,  // no resizing
                        views::GridLayout::USE_PREF,
                        0,  // no fixed width
                        title->GetPreferredSize().width());
  column_set->AddPaddingColumn(0, content_insets.right());
  layout->StartRow(0, 0);
  layout->AddView(icon, 1, 1);
  layout->AddView(title, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::CENTER);
  layout->StartRow(1, 0);
  layout->AddView(detail, 4, 1);

  layout->AddPaddingRow(0, 10);
  const gfx::Insets button_row_insets =
      provider->GetInsetsMetric(views::INSETS_DIALOG_BUTTON_ROW);
  SetPreferredSize(
      gfx::Size(preferred_content_width + button_row_insets.width(),
                contents_view_->GetPreferredSize().height()));
}

base::string16 PasswordProtectionWarningDialog::GetTitle() {
  return softer_warning_
             ? l10n_util::GetStringUTF16(
                   IDS_PASSWORD_REUSE_WARNING_TITLE_SOFTER)
             : l10n_util::GetStringUTF16(IDS_PASSWORD_REUSE_WARNING_TITLE);
}

base::string16 PasswordProtectionWarningDialog::GetDialogMessage() {
  return softer_warning_
             ? l10n_util::GetStringUTF16(
                   IDS_PASSWORD_REUSE_WARNING_DETAIL_SOFTER)
             : l10n_util::GetStringUTF16(IDS_PASSWORD_REUSE_WARNING_DETAIL);
}

bool PasswordProtectionWarningDialog::Accept() {
  done_.Run(PasswordProtectionService::CHANGE_PASSWORD);
  return true;
}

bool PasswordProtectionWarningDialog::Cancel() {
  done_.Run(PasswordProtectionService::IGNORE_WARNING);
  // click ignore
  return true;
}

// Navigate away or hit "ESC" key.
bool PasswordProtectionWarningDialog::Close() {
  done_.Run(PasswordProtectionService::CLOSE);
  return true;
}

bool PasswordProtectionWarningDialog::ShouldShowCloseButton() const {
  return false;
}

ui::ModalType PasswordProtectionWarningDialog::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

void PasswordProtectionWarningDialog::Layout() {
  contents_view_->SetBounds(0, 0, width(), height());
  DialogDelegateView::Layout();
}

int PasswordProtectionWarningDialog::GetDefaultDialogButton() const {
  return ui::DIALOG_BUTTON_OK;
}

base::string16 PasswordProtectionWarningDialog::GetDialogButtonLabel(
    ui::DialogButton button) const {
  switch (button) {
    case ui::DIALOG_BUTTON_OK:
      return l10n_util::GetStringUTF16(IDS_PASSWORD_REUSE_WARNING_ACCEPT);
    case ui::DIALOG_BUTTON_CANCEL:
      return l10n_util::GetStringUTF16(IDS_PASSWORD_REUSE_WARNING_CANCEL);
    default:
      return DialogDelegate::GetDialogButtonLabel(button);
  }
}

}  // namespace safe_browsing
