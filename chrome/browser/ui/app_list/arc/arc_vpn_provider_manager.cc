// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_vpn_provider_manager.h"

#include "chrome/browser/ui/app_list/arc/arc_vpn_provider_manager_factory.h"

namespace app_list {

namespace {

// Checks if a package is an ArcVPNProvider.
bool IsPackageArcVPNProvider(const std::string& package_name,
                             const ArcAppListPrefs* arc_app_list_prefs) {
  auto package_info = arc_app_list_prefs->GetPackage(package_name);
  return package_info && package_info->vpn_provider;
}

std::unique_ptr<ArcVpnProviderManager::ArcVpnProvider>
CreateArcVPNProviderFromApp(const std::string& app_id,
                            const ArcAppListPrefs* arc_app_list_prefs) {
  auto app_info = arc_app_list_prefs->GetApp(app_id);
  if (!app_info ||
      !IsPackageArcVPNProvider(app_info->package_name, arc_app_list_prefs))
    return nullptr;

  return std::make_unique<ArcVpnProviderManager::ArcVpnProvider>(
      app_info->name, app_info->package_name, app_id,
      app_info->last_launch_time.is_null() ? app_info->install_time
                                           : app_info->last_launch_time);
}

std::unique_ptr<ArcVpnProviderManager::ArcVpnProvider>
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

ArcVpnProviderManager::ArcVpnProviderManager(
    ArcAppListPrefs* arc_app_list_prefs)
    : arc_app_list_prefs_(arc_app_list_prefs) {
  arc_app_list_prefs_->AddObserver(this);
}

ArcVpnProviderManager::~ArcVpnProviderManager() {
  if (arc_app_list_prefs_)
    arc_app_list_prefs_->RemoveObserver(this);
}

// static
ArcVpnProviderManager* ArcVpnProviderManager::Get(
    content::BrowserContext* context) {
  return ArcVpnProviderManagerFactory::GetForBrowserContext(context);
}

// static
ArcVpnProviderManager* ArcVpnProviderManager::Create(
    content::BrowserContext* context) {
  ArcAppListPrefs* arc_app_list_prefs = ArcAppListPrefs::Get(context);
  if (!arc_app_list_prefs)
    return nullptr;

  return new ArcVpnProviderManager(arc_app_list_prefs);
}

void ArcVpnProviderManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ArcVpnProviderManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

std::vector<std::unique_ptr<ArcVpnProviderManager::ArcVpnProvider>>
ArcVpnProviderManager::GetArcVpnProviders() {
  std::vector<std::unique_ptr<ArcVpnProvider>> arc_vpn_providers;
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return arc_vpn_providers;

  const std::vector<std::string> packge_names =
      arc_app_list_prefs_->GetPackagesFromPrefs();
  for (const auto& package_name : packge_names) {
    if (!IsPackageArcVPNProvider(package_name, arc_app_list_prefs_))
      continue;
    auto arc_vpn_provider =
        CreateArcVPNProviderFromPackage(package_name, arc_app_list_prefs_);
    if (!arc_vpn_provider)
      continue;
    arc_vpn_providers.push_back(std::move(arc_vpn_provider));
  }

  return arc_vpn_providers;
}

void ArcVpnProviderManager::OnAppNameUpdated(const std::string& app_id,
                                             const std::string& name) {
  MaybeNotifyArcVpnProviderUpdate(app_id);
}

void ArcVpnProviderManager::OnAppLastLaunchTimeUpdated(
    const std::string& app_id) {
  MaybeNotifyArcVpnProviderUpdate(app_id);
}

void ArcVpnProviderManager::OnPackageInstalled(
    const arc::mojom::ArcPackageInfo& package_info) {
  auto arc_vpn_provider = CreateArcVPNProviderFromPackage(
      package_info.package_name, arc_app_list_prefs_);
  if (!arc_vpn_provider)
    return;

  for (auto& observer : observer_list_)
    observer.OnArcVpnProviderUpdated(arc_vpn_provider.get());
}

void ArcVpnProviderManager::OnPackageRemoved(const std::string& package_name,
                                             bool uninstalled) {
  for (auto& observer : observer_list_)
    observer.OnArcVpnProviderRemoved(package_name);
}

void ArcVpnProviderManager::OnPackageListInitialRefreshed() {
  auto arc_vpn_providers = GetArcVpnProviders();

  for (auto& observer : observer_list_)
    observer.OnArcVpnProvidersRefreshed(arc_vpn_providers);
}

void ArcVpnProviderManager::MaybeNotifyArcVpnProviderUpdate(
    const std::string& app_id) {
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return;

  auto arc_vpn_provider =
      CreateArcVPNProviderFromApp(app_id, arc_app_list_prefs_);
  if (!arc_vpn_provider)
    return;

  for (auto& observer : observer_list_)
    observer.OnArcVpnProviderUpdated(arc_vpn_provider.get());
}

ArcVpnProviderManager::ArcVpnProvider::ArcVpnProvider(
    const std::string app_name,
    const std::string package_name,
    const std::string app_id,
    base::Time& last_launch_time)
    : app_name(app_name),
      package_name(package_name),
      app_id(app_id),
      last_launch_time(last_launch_time) {}

}  // namespace app_list
