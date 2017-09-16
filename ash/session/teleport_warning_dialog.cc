// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/session/teleport_warning_dialog.h"

#include "ash/shell.h"
#include "base/macros.h"
//#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"

// Default width of the dialog.
static const int kDefaultWidth = 448;

TeleportWarningDialog::TeleportWarningDialog(OnAcceptCallback callback)
    : on_accept_(std::move(callback)) {}

TeleportWarningDialog::~TeleportWarningDialog() {}

// static
void TeleportWarningDialog::Show(OnAcceptCallback callback) {
  TeleportWarningDialog* dialog_view =
      new TeleportWarningDialog(std::move(callback));
  dialog_view->InitDialog();
  views::DialogDelegate::CreateDialogWidget(
      dialog_view, ash::Shell::GetRootWindowForNewWindows(), NULL);
  views::Widget* widget = dialog_view->GetWidget();
  DCHECK(widget);
  widget->Show();
}

bool TeleportWarningDialog::Cancel() {
  std::move(on_accept_).Run(false, false);
  return true;
}

bool TeleportWarningDialog::Accept() {
  std::move(on_accept_).Run(true, no_show_checkbox_->checked());
  return true;
}

views::View* TeleportWarningDialog::CreateExtraView() {
  no_show_checkbox_ = new views::Checkbox(
      l10n_util::GetStringUTF16(IDS_VISIT_DESKTOP_WARNING_SHOW_DISMISS));
  no_show_checkbox_->SetChecked(true);
  return no_show_checkbox_;
}

ui::ModalType TeleportWarningDialog::GetModalType() const {
  return ui::MODAL_TYPE_SYSTEM;
}

base::string16 TeleportWarningDialog::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_VISIT_DESKTOP_WARNING_TITLE);
}

gfx::Size TeleportWarningDialog::CalculatePreferredSize() const {
  return gfx::Size(
      kDefaultWidth,
      GetLayoutManager()->GetPreferredHeightForWidth(this, kDefaultWidth));
}

void TeleportWarningDialog::InitDialog() {
  SetBorder(
      views::CreateEmptyBorder(views::LayoutProvider::Get()->GetInsetsMetric(
          views::INSETS_DIALOG_TITLE)));
  SetLayoutManager(new views::FillLayout());

  // Explanation string
  views::Label* label = new views::Label(
      l10n_util::GetStringUTF16(IDS_VISIT_DESKTOP_WARNING_MESSAGE));
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(label);
}
