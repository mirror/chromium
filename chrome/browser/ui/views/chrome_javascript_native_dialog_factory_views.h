// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CHROME_JAVASCRIPT_NATIVE_DIALOG_FACTORY_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_CHROME_JAVASCRIPT_NATIVE_DIALOG_FACTORY_VIEWS_H_

#include "base/macros.h"
#include "components/app_modal/javascript_native_dialog_factory.h"

class ChromeJavaScriptNativeDialogViewsFactory
    : public app_modal::JavaScriptNativeDialogFactory {
 public:
  ChromeJavaScriptNativeDialogViewsFactory();
  ~ChromeJavaScriptNativeDialogViewsFactory() override;

  app_modal::NativeAppModalDialog* CreateNativeJavaScriptDialog(
      app_modal::JavaScriptAppModalDialog* dialog) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeJavaScriptNativeDialogViewsFactory);
};

#endif  // CHROME_BROWSER_UI_VIEWS_CHROME_JAVASCRIPT_NATIVE_DIALOG_FACTORY_VIEWS_H_
