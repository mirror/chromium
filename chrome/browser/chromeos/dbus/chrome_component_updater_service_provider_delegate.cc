// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/chrome_component_updater_service_provider_delegate.h"

#include "base/task_scheduler/post_task.h"
#include "chrome/browser/component_updater/cros_component_installer.h"

namespace chromeos {

namespace {

void OnLoadComponent(
    ChromeComponentUpdaterServiceProviderDelegate::LoadCallback load_callback,
    component_updater::CrOSComponentManager::Error error,
    const base::FilePath& mount_path) {
  base::PostTask(FROM_HERE,
                 base::BindOnce(std::move(load_callback), mount_path));
}

}  // namespace

ChromeComponentUpdaterServiceProviderDelegate::
    ChromeComponentUpdaterServiceProviderDelegate() {}

ChromeComponentUpdaterServiceProviderDelegate::
    ~ChromeComponentUpdaterServiceProviderDelegate() {}

void ChromeComponentUpdaterServiceProviderDelegate::LoadComponent(
    const std::string& name,
    bool mount,
    LoadCallback load_callback) {
  g_browser_process->platform_part()->cros_component_manager()->Load(
      name,
      mount ? component_updater::CrOSComponentManager::MountPolicy::kMount
            : component_updater::CrOSComponentManager::MountPolicy::kDontMount,
      base::BindOnce(OnLoadComponent, std::move(load_callback)));
}

bool ChromeComponentUpdaterServiceProviderDelegate::UnloadComponent(
    const std::string& name) {
  return g_browser_process->platform_part()->cros_component_manager()->Unload(
      name);
}

}  // namespace chromeos
