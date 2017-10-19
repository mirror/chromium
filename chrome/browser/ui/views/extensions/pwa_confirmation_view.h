// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_PWA_CONFIRMATION_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_PWA_CONFIRMATION_VIEW_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/web_application_info.h"
#include "ui/views/window/dialog_delegate.h"

// PWAConfirmationView provides views for editing the details to
// install PWA with.
class PWAConfirmationView : public views::DialogDelegateView {
 public:
  PWAConfirmationView(const WebApplicationInfo& web_app_info,
                      chrome::AppInstallationAcceptanceCallback callback);
  ~PWAConfirmationView() override;

 private:
  // Overridden from views::WidgetDelegate:
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  void WindowClosing() override;

  // Overriden from views::DialogDelegateView:
  bool Accept() override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;

  // Overridden from views::View:
  gfx::Size GetMinimumSize() const override;

  void CreateDialog(const base::string16& title,
                    const base::string16& origin,
                    const std::vector<WebApplicationInfo::IconInfo>& icons);

  // The WebApplicationInfo that describes the app to be installed.
  WebApplicationInfo web_app_info_;

  // The callback to be invoked when the dialog is completed.
  chrome::AppInstallationAcceptanceCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(PWAConfirmationView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_PWA_CONFIRMATION_VIEW_H_
