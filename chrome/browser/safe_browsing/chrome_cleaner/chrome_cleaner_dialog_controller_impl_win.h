// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_DIALOG_CONTROLLER_IMPL_WIN_H_
#define CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_DIALOG_CONTROLLER_IMPL_WIN_H_

#include <set>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_dialog_controller_win.h"
#include "ui/views/widget/widget_observer.h"

class Browser;

namespace views {
class Widget;
}

namespace safe_browsing {

class ChromeCleanerDialogControllerImpl
    : public ChromeCleanerDialogController,
      public ChromeCleanerController::Observer,
      public views::WidgetObserver {
 public:
  // An instance should only be created when |cleaner_controller| is in the
  // kScanning state.
  explicit ChromeCleanerDialogControllerImpl(
      ChromeCleanerController* cleaner_controller);

  // ChromeCleanerDialogController overrides.
  void DialogShown() override;
  void Accept(bool logs_enabled) override;
  void Cancel() override;
  void Close() override;
  void DetailsButtonClicked(bool logs_enabled) override;
  void SetLogsEnabled(bool logs_enabled) override;
  bool LogsEnabled() override;

  // ChromeCleanerController::Observer overrides.
  void OnIdle(ChromeCleanerController::IdleReason idle_reason) override;
  void OnScanning() override;
  void OnInfected(const std::set<base::FilePath>& files_to_delete) override;
  void OnCleaning(const std::set<base::FilePath>& files_to_delete) override;
  void OnRebootRequired() override;

  // views::WidgetObserver overrides.
  void OnWidgetClosing(views::Widget* widget) override;

 protected:
  ~ChromeCleanerDialogControllerImpl() override;

 private:
  // Called when the cleaner controller leaves the infected state without the
  // user interacting with the dialog. This can happen due to IPC errors or if
  // the user interacts with the Chrome Cleaner's webui card in the settings
  // page in a different window.
  void Abort();
  void OnInteractionDone();

  ChromeCleanerController* cleaner_controller_ = nullptr;
  bool dialog_shown_ = false;
  views::Widget* dialog_widget_ = nullptr;
  bool aborted_ = false;
  Browser* browser_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ChromeCleanerDialogControllerImpl);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_CHROME_CLEANER_DIALOG_CONTROLLER_IMPL_WIN_H_
