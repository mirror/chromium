// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/download/download_in_progress_dialog_view.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/border.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

// static
void DownloadInProgressDialogView::Show(
    gfx::NativeWindow parent,
    int download_count,
    Browser::DownloadClosePreventionType dialog_type,
    bool app_modal,
    const base::Callback<void(bool)>& callback) {
  DownloadInProgressDialogView* window = new DownloadInProgressDialogView(
      download_count, dialog_type, app_modal, callback);
  constrained_window::CreateBrowserModalDialogViews(window, parent)->Show();
}

DownloadInProgressDialogView::DownloadInProgressDialogView(
    int download_count,
    Browser::DownloadClosePreventionType dialog_type,
    bool app_modal,
    const base::Callback<void(bool)>& callback)
    : download_count_(download_count),
      app_modal_(app_modal),
      callback_(callback),
      message_box_view_(nullptr) {
  // This dialog should have been created within the same thread invocation
  // as the original test, so it's never ok to close.
  DCHECK_NE(Browser::DOWNLOAD_CLOSE_OK, dialog_type);
  base::string16 explanation_text = l10n_util::GetStringUTF16(
      dialog_type == Browser::DOWNLOAD_CLOSE_BROWSER_SHUTDOWN
          ? IDS_ABANDON_DOWNLOAD_DIALOG_BROWSER_MESSAGE
          : IDS_ABANDON_DOWNLOAD_DIALOG_INCOGNITO_MESSAGE);
  message_box_view_ = new views::MessageBoxView(
      views::MessageBoxView::InitParams(explanation_text));
  chrome::RecordDialogCreation(chrome::DialogIdentifier::DOWNLOAD_IN_PROGRESS);
}

DownloadInProgressDialogView::~DownloadInProgressDialogView() {}

int DownloadInProgressDialogView::GetDefaultDialogButton() const {
  return ui::DIALOG_BUTTON_CANCEL;
}

base::string16 DownloadInProgressDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(
      button == ui::DIALOG_BUTTON_OK
          ? IDS_ABANDON_DOWNLOAD_DIALOG_EXIT_BUTTON
          : IDS_ABANDON_DOWNLOAD_DIALOG_CONTINUE_BUTTON);
}

bool DownloadInProgressDialogView::Cancel() {
  callback_.Run(false /* cancel_downloads */);
  return true;
}

bool DownloadInProgressDialogView::Accept() {
  callback_.Run(true /* cancel_downloads */);
  return true;
}

ui::ModalType DownloadInProgressDialogView::GetModalType() const {
  return app_modal_ ? ui::MODAL_TYPE_SYSTEM : ui::MODAL_TYPE_WINDOW;
}

bool DownloadInProgressDialogView::ShouldShowCloseButton() const {
  return false;
}

base::string16 DownloadInProgressDialogView::GetWindowTitle() const {
  return l10n_util::GetPluralStringFUTF16(IDS_ABANDON_DOWNLOAD_DIALOG_TITLE,
                                          download_count_);
}

void DownloadInProgressDialogView::DeleteDelegate() {
  delete this;
}

views::Widget* DownloadInProgressDialogView::GetWidget() {
  return message_box_view_->GetWidget();
}

const views::Widget* DownloadInProgressDialogView::GetWidget() const {
  return message_box_view_->GetWidget();
}

views::View* DownloadInProgressDialogView::GetContentsView() {
  return message_box_view_;
}
