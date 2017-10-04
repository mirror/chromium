// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_cleaner_reboot_dialog_win.h"

#include "base/strings/string16.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_reboot_dialog_controller_win.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"

namespace chrome {

void ShowChromeCleanerRebootPrompt(
    Browser* browser,
    safe_browsing::ChromeCleanerRebootDialogController* dialog_controller) {
  DCHECK(browser);
  DCHECK(dialog_controller);

  ChromeCleanerRebootDialog* dialog =
      new ChromeCleanerRebootDialog(dialog_controller);
  dialog->Show(browser);
}

}  // namespace chrome

namespace {
constexpr int kDialogWidth = 448;
}  // namespace

enum class ChromeCleanerRebootDialog::DialogInteractionResult {
  kAccept,
  kCancel,
  kClose,
};

ChromeCleanerRebootDialog::ChromeCleanerRebootDialog(
    safe_browsing::ChromeCleanerRebootDialogController* dialog_controller)
    : browser_(nullptr), dialog_controller_(dialog_controller) {
  DCHECK(dialog_controller_);

  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));
  SetLayoutManager(new views::FillLayout());
}

ChromeCleanerRebootDialog::~ChromeCleanerRebootDialog() {
  // Make sure the controller is correctly notified in case the dialog widget is
  // closed by some other means than the dialog buttons.
  if (dialog_controller_)
    dialog_controller_->Close();
}

void ChromeCleanerRebootDialog::Show(Browser* browser) {
  DCHECK(browser);
  DCHECK(!browser_);
  DCHECK(dialog_controller_);

  browser_ = browser;
  constrained_window::CreateBrowserModalDialogViews(
      this, browser_->window()->GetNativeWindow())
      ->Show();
  dialog_controller_->DialogShown();
}

// WidgetDelegate overrides.

ui::ModalType ChromeCleanerRebootDialog::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 ChromeCleanerRebootDialog::GetWindowTitle() const {
  DCHECK(dialog_controller_);
  // TODO(b/770749) Use a translatable string.
  return L"To finish removing harmful software, restart your computer";
}

// DialogDelegate overrides.

base::string16 ChromeCleanerRebootDialog::GetDialogButtonLabel(
    ui::DialogButton button) const {
  DCHECK(button == ui::DIALOG_BUTTON_OK || button == ui::DIALOG_BUTTON_CANCEL);
  DCHECK(dialog_controller_);

  // TODO(b/770749) Use a translatable string.
  return button == ui::DIALOG_BUTTON_OK
             ? L"Restart computer"
             : DialogDelegate::GetDialogButtonLabel(button);
}

bool ChromeCleanerRebootDialog::Accept() {
  HandleDialogInteraction(DialogInteractionResult::kAccept);
  return true;
}

bool ChromeCleanerRebootDialog::Cancel() {
  HandleDialogInteraction(DialogInteractionResult::kCancel);
  return true;
}

bool ChromeCleanerRebootDialog::Close() {
  HandleDialogInteraction(DialogInteractionResult::kClose);
  return true;
}

// View overrides.

gfx::Size ChromeCleanerRebootDialog::CalculatePreferredSize() const {
  return gfx::Size(kDialogWidth, GetHeightForWidth(kDialogWidth));
}

void ChromeCleanerRebootDialog::HandleDialogInteraction(
    DialogInteractionResult result) {
  if (!dialog_controller_)
    return;

  switch (result) {
    case DialogInteractionResult::kAccept:
      dialog_controller_->Accept();
      break;
    case DialogInteractionResult::kCancel:
      dialog_controller_->Cancel();
      break;
    case DialogInteractionResult::kClose:
      dialog_controller_->Close();
      break;
  }
  dialog_controller_ = nullptr;
}
