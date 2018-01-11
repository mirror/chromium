// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_DELEGATES_SHELL_KIOSK_DELEGATE_H_
#define EXTENSIONS_SHELL_BROWSER_DELEGATES_SHELL_KIOSK_DELEGATE_H_

#include "extensions/browser/kiosk/kiosk_delegate.h"
#include "extensions/common/extension_id.h"

namespace extensions {

// Delegate in AppShell that provides an extension/app API with Kiosk mode
// functionality.
class ShellKioskDelegate : public KioskDelegate {
 public:
  ShellKioskDelegate();
  ~ShellKioskDelegate() override;

  // KioskDelegate overrides:
  bool IsPrimaryKioskApp(const ExtensionId& id) const override;
  bool IsAutoLaunchedKioskApp(const ExtensionId& id) const override;

  void set_primary_app_id(const extensions::ExtensionId& id) {
    primary_app_id_ = id;
  }

 private:
  ExtensionId primary_app_id_;
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_DELEGATES_SHELL_KIOSK_DELEGATE_H_
