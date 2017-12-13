// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/chrome_component_updater_service_provider_delegate.h"

#include "chrome/browser/component_updater/cros_component_installer.h"

namespace chromeos {

ChromeComponentUpdaterServiceProviderDelegate::
    ChromeComponentUpdaterServiceProviderDelegate() {}

ChromeComponentUpdaterServiceProviderDelegate::
    ~ChromeComponentUpdaterServiceProviderDelegate() {}

void ChromeComponentUpdaterServiceProviderDelegate::LoadComponent(
    const std::string& name,
    base::OnceCallback<void(const std::string&)> load_callback) {
  g_browser_process->platform_part()->cros_component()->LoadComponent(
      name, std::move(load_callback));
}

bool ChromeComponentUpdaterServiceProviderDelegate::UnloadComponent(
    const std::string& name) {
  return g_browser_process->platform_part()->cros_component()->UnloadComponent(
      name);
}

}  // namespace chromeos
