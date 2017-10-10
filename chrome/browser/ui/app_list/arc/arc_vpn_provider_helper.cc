// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_vpn_provider_helper.h"

#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"

namespace {

// Checks if a package is an ArcVPNProvider.
bool IsPackageArcVPNProvider(const std::string& package_name,
                             const ArcAppListPrefs* arc_app_list_prefs) {
  auto package_info = arc_app_list_prefs->GetPackage(package_name);
  return package_info && package_info->vpn_provider;
}

std::unique_ptr<ArcVpnProviderHelper::ArcVpnProvider>
CreateArcVPNProviderFromApp(const std::string& app_id,
                            const ArcAppListPrefs* arc_app_list_prefs) {
  auto app_info = arc_app_list_prefs->GetApp(app_id);
  if (!app_info ||
      !IsPackageArcVPNProvider(app_info->package_name, arc_app_list_prefs))
    return nullptr;

  return std::make_unique<ArcVpnProviderHelper::ArcVpnProvider>(
      app_info->name, app_info->package_name, app_id,
      app_info->last_launch_time.is_null() ? app_info->install_time
                                           : app_info->last_launch_time);
}

std::unique_ptr<ArcVpnProviderHelper::ArcVpnProvider>
CreateArcVPNProviderFromPackage(const std::string& package_name,
                                const ArcAppListPrefs* arc_app_list_prefs) {
  auto package_apps = arc_app_list_prefs->GetAppsForPackage(package_name);
  if (package_apps.size() < 1)
    return nullptr;
  // Use first launchable of the package.
  // Todo(lgcheng@) Add UMA status here. If we observe VPN package has multiple
  // launchables, find correct launchable here.
  std::string app_id = *package_apps.begin();
  return CreateArcVPNProviderFromApp(app_id, arc_app_list_prefs);
}

}  // namespace

ArcVpnProviderHelper::ArcVpnProviderHelper(ArcAppListPrefs* arc_app_list_prefs)
    : arc_app_list_prefs_(arc_app_list_prefs) {}

ArcVpnProviderHelper::~ArcVpnProviderHelper() {}

void ArcVpnProviderHelper::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ArcVpnProviderHelper::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void ArcVpnProviderHelper::UpdateAppName(const std::string& app_id) {
  NotifyArcVpnProviderUpdate(app_id);
}

void ArcVpnProviderHelper::UpdatePackageInstalled(
    const std::string& package_name) {
  auto arc_vpn_provider =
      CreateArcVPNProviderFromPackage(package_name, arc_app_list_prefs_);
  if (!arc_vpn_provider)
    return;

  for (auto& observer : observer_list_)
    observer.OnArcVpnProviderUpdated(std::move(arc_vpn_provider));
}

void ArcVpnProviderHelper::UpdatePackageListInitialRefreshed() {
  std::vector<std::unique_ptr<ArcVpnProvider>> arc_vpn_providers;
  std::vector<std::string> packge_names =
      arc_app_list_prefs_->GetPackagesFromPrefs();
  for (auto package_name : packge_names) {
    if (!IsPackageArcVPNProvider(package_name, arc_app_list_prefs_))
      continue;
    auto arc_vpn_provider =
        CreateArcVPNProviderFromPackage(package_name, arc_app_list_prefs_);
    if (arc_vpn_provider) {
      arc_vpn_providers.push_back(std::move(arc_vpn_provider));
    }
  }

  for (auto& observer : observer_list_)
    observer.OnArcVpnProvidersRefreshed(arc_vpn_providers);
}

void ArcVpnProviderHelper::UpdateLastLauchTime(const std::string& app_id) {
  NotifyArcVpnProviderUpdate(app_id);
}

void ArcVpnProviderHelper::NotifyArcVpnProviderUpdate(
    const std::string& app_id) {
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return;

  auto arc_vpn_provider =
      CreateArcVPNProviderFromApp(app_id, arc_app_list_prefs_);
  if (!arc_vpn_provider)
    return;

  for (auto& observer : observer_list_)
    observer.OnArcVpnProviderUpdated(std::move(arc_vpn_provider));
}

ArcVpnProviderHelper::ArcVpnProvider::ArcVpnProvider(
    const std::string app_name,
    const std::string package_name,
    const std::string app_id,
    base::Time& last_launch_time)
    : app_name(app_name),
      package_name(package_name),
      app_id(app_id),
      last_launch_time(last_launch_time) {}
