// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CHROME_CLEANER_REBOOT_DIALOG_WIN_H_
#define CHROME_BROWSER_UI_VIEWS_CHROME_CLEANER_REBOOT_DIALOG_WIN_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/dialog_delegate.h"

class Browser;

namespace safe_browsing {
class ChromeCleanerRebootDialogController;
}

// A modal dialog asking the user if they want to remove harmful software from
// their computers by running the Chrome Cleanup tool.
//
// The strings and icons used in the dialog are provided by a
// |ChromeCleanerRebootDialogController| object, which will also receive
// information about how the user interacts with the dialog. The controller
// object owns itself and will delete itself once it has received information
// about the user's interaction with the dialog. See the
// |ChromeCleanerRebootDialogController| class's description for more details.
class ChromeCleanerRebootDialog : public views::DialogDelegateView {
 public:
  // The |controller| object manages its own lifetime and is not owned by
  // |ChromeCleanerRebootDialog|. See the description of the
  // |ChromeCleanerRebootDialogController| class for details.
  ChromeCleanerRebootDialog(
      safe_browsing::ChromeCleanerRebootDialogController* dialog_controller);
  ~ChromeCleanerRebootDialog() override;

  void Show(Browser* browser);

  // views::WidgetDelegate overrides.
  ui::ModalType GetModalType() const override;
  // bool ShouldShowWindowTitle() const override;
  base::string16 GetWindowTitle() const override;

  // views::DialogDelegate overrides.
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool Accept() override;
  bool Cancel() override;
  bool Close() override;

  // views::View overrides.
  gfx::Size CalculatePreferredSize() const override;

 private:
  enum class DialogInteractionResult;

  void HandleDialogInteraction(DialogInteractionResult result);

  Browser* browser_;
  // The pointer will be set to nullptr once the controller has been notified of
  // user interaction since the controller can delete itself after that point.
  safe_browsing::ChromeCleanerRebootDialogController* dialog_controller_;

  DISALLOW_COPY_AND_ASSIGN(ChromeCleanerRebootDialog);
};

#endif  // CHROME_BROWSER_UI_VIEWS_CHROME_CLEANER_REBOOT_DIALOG_WIN_H_
