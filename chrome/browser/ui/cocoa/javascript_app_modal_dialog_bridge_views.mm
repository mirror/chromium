// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/javascript_app_modal_dialog_cocoa.h"
#include "chrome/browser/ui/javascript_dialogs/chrome_javascript_native_dialog_factory.h"
#include "chrome/browser/ui/views/chrome_javascript_native_dialog_factory_views.h"
#include "components/app_modal/javascript_app_modal_dialog.h"
#include "components/app_modal/javascript_dialog_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/material_design/material_design_controller.h"

namespace {

class ChromeJavaScriptNativeDialogCocoaFactory
    : public app_modal::JavaScriptNativeDialogFactory {
 public:
  ChromeJavaScriptNativeDialogCocoaFactory() {}
  ~ChromeJavaScriptNativeDialogCocoaFactory() override {}

 private:
  app_modal::NativeAppModalDialog* CreateNativeJavaScriptDialog(
      app_modal::JavaScriptAppModalDialog* dialog) override {
    app_modal::NativeAppModalDialog* d =
        new JavaScriptAppModalDialogCocoa(dialog);
    dialog->web_contents()->GetDelegate()->ActivateContents(
        dialog->web_contents());
    return d;
  }

  DISALLOW_COPY_AND_ASSIGN(ChromeJavaScriptNativeDialogCocoaFactory);
};

}  // namespace

void InstallChromeJavaScriptNativeDialogFactory() {
  std::unique_ptr<app_modal::JavaScriptNativeDialogFactory> factory;
  if (ui::MaterialDesignController::IsSecondaryUiMaterial())
    factory.reset(new ChromeJavaScriptNativeDialogViewsFactory);
  else
    factory.reset(new ChromeJavaScriptNativeDialogCocoaFactory);
  app_modal::JavaScriptDialogManager::GetInstance()->SetNativeDialogFactory(
      std::move(factory));
}
