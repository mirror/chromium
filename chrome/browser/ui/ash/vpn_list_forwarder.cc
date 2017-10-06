// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/vpn_list_forwarder.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

bool IsVPNProvider(const extensions::Extension* extension) {
  return extension->permissions_data()->HasAPIPermission(
      extensions::APIPermission::kVpnProvider);
}

// Checks if a package is an ArcVPNProvider.
bool IsPackageArcVPNProvider(const std::string& package_name,
                             const ArcAppListPrefs* arc_app_list_prefs) {
  auto package_info = arc_app_list_prefs->GetPackage(package_name);
  return package_info && package_info->vpn_provider;
}

ash::mojom::ArcVpnProviderPtr CreateArcVPNProviderFromApp(
    const std::string& app_id,
    const ArcAppListPrefs* arc_app_list_prefs) {
  auto app_info = arc_app_list_prefs->GetApp(app_id);
  if (!app_info ||
      !IsPackageArcVPNProvider(app_info->package_name, arc_app_list_prefs))
    return nullptr;

  auto arc_vpn_provider = ash::mojom::ArcVpnProvider::New();
  arc_vpn_provider->package_name = app_info->package_name;
  arc_vpn_provider->app_name = app_info->name;
  arc_vpn_provider->app_id = app_id;
  arc_vpn_provider->last_launch_time = app_info->last_launch_time.is_null()
                                           ? app_info->install_time
                                           : app_info->last_launch_time;

  return arc_vpn_provider;
}

ash::mojom::ArcVpnProviderPtr CreateArcVPNProviderFromPackage(
    const std::string& package_name,
    const ArcAppListPrefs* arc_app_list_prefs) {
  auto package_apps = arc_app_list_prefs->GetAppsForPackage(package_name);
  if (package_apps.size() < 1)
    return nullptr;
  // Use first launchable of the package.
  // Todo(lgcheng@) If we observe VPN package has multiple launchables, use
  // latest used launchable here.
  std::string app_id = *package_apps.begin();
  return CreateArcVPNProviderFromApp(app_id, arc_app_list_prefs);
}

Profile* GetProfileForPrimaryUser() {
  const user_manager::User* const primary_user =
      user_manager::UserManager::Get()->GetPrimaryUser();
  if (!primary_user)
    return nullptr;

  return chromeos::ProfileHelper::Get()->GetProfileByUser(primary_user);
}

// Connects to the VpnList mojo interface in ash.
ash::mojom::VpnListPtr ConnectToVpnList() {
  ash::mojom::VpnListPtr vpn_list;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &vpn_list);
  return vpn_list;
}

}  // namespace

VpnListForwarder::VpnListForwarder() : weak_factory_(this) {
  if (user_manager::UserManager::Get()->GetPrimaryUser()) {
    // If a user is logged in, start observing the primary user's extension
    // registry immediately.
    AttachToPrimaryUserProfile();
  } else {
    // If no user is logged in, wait until the first user logs in (thus becoming
    // the primary user) and a profile is created for that user.
    registrar_.Add(this, chrome::NOTIFICATION_PROFILE_CREATED,
                   content::NotificationService::AllSources());
  }
}

VpnListForwarder::~VpnListForwarder() {
  if (extension_registry_)
    extension_registry_->RemoveObserver(this);
  if (arc_app_list_prefs_)
    arc_app_list_prefs_->RemoveObserver(this);
  vpn_list_ = nullptr;
}

void VpnListForwarder::OnAppNameUpdated(const std::string& id,
                                        const std::string& name) {
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return;

  auto arc_vpn_provider = CreateArcVPNProviderFromApp(id, arc_app_list_prefs_);
  if (arc_vpn_provider)
    vpn_list_->AddOrUpdateArcVPNProvider(std::move(arc_vpn_provider));
}

void VpnListForwarder::OnLastLaunchTimeUpdated(const std::string& app_id) {
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return;

  auto arc_vpn_provider =
      CreateArcVPNProviderFromApp(app_id, arc_app_list_prefs_);
  if (arc_vpn_provider)
    vpn_list_->AddOrUpdateArcVPNProvider(std::move(arc_vpn_provider));
}

void VpnListForwarder::OnPackageInstalled(
    const arc::mojom::ArcPackageInfo& package_info) {
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return;

  auto arc_vpn_provider = CreateArcVPNProviderFromPackage(
      package_info.package_name, arc_app_list_prefs_);
  if (arc_vpn_provider)
    vpn_list_->AddOrUpdateArcVPNProvider(std::move(arc_vpn_provider));
}

// Package information is not gurranted to be available at this moment. Forward
// event to ash to see if uninstalled package matches any of the cached
// providers.
void VpnListForwarder::OnPackageRemoved(const std::string& package_name,
                                        bool uninstalled) {
  if (!arc_app_list_prefs_->package_list_initial_refreshed())
    return;

  vpn_list_->RemoveArcVPNProvider(package_name);
}

void VpnListForwarder::OnPackageListInitialRefreshed() {
  UpdateArcVPNProviders();
}

void VpnListForwarder::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (IsVPNProvider(extension))
    UpdateVPNProviders();
}

void VpnListForwarder::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (IsVPNProvider(extension))
    UpdateVPNProviders();
}

void VpnListForwarder::OnShutdown(extensions::ExtensionRegistry* registry) {
  DCHECK(extension_registry_);
  extension_registry_->RemoveObserver(this);
  extension_registry_ = nullptr;
}

void VpnListForwarder::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_CREATED, type);
  const Profile* const profile = content::Source<Profile>(source).ptr();
  if (!chromeos::ProfileHelper::Get()->IsPrimaryProfile(profile)) {
    // If the profile that was just created does not belong to the primary user
    // (e.g. login profile), ignore it.
    return;
  }

  // The first user logged in (thus becoming the primary user) and a profile was
  // created for that user. Stop observing profile creation. Wait one message
  // loop cycle to allow other code which observes the
  // chrome::NOTIFICATION_PROFILE_CREATED notification to finish initializing
  // the profile, then start observing the primary user's extension registry.
  registrar_.RemoveAll();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&VpnListForwarder::AttachToPrimaryUserProfile,
                                weak_factory_.GetWeakPtr()));
}

void VpnListForwarder::UpdateVPNProviders() {
  DCHECK(extension_registry_);

  std::vector<ash::mojom::ThirdPartyVpnProviderPtr> third_party_providers;
  for (const auto& extension : extension_registry_->enabled_extensions()) {
    if (!IsVPNProvider(extension.get()))
      continue;

    ash::mojom::ThirdPartyVpnProviderPtr provider =
        ash::mojom::ThirdPartyVpnProvider::New();
    provider->name = extension->name();
    provider->extension_id = extension->id();
    third_party_providers.push_back(std::move(provider));
  }

  // Ash starts without any third-party providers. If we've never sent one then
  // there's no need to send an empty list. This case commonly occurs on startup
  // when the user has no third-party VPN extensions installed.
  if (!sent_providers_ && third_party_providers.empty())
    return;

  vpn_list_->SetThirdPartyVpnProviders(std::move(third_party_providers));

  sent_providers_ = true;
}

void VpnListForwarder::UpdateArcVPNProviders() {
  std::vector<ash::mojom::ArcVpnProviderPtr> arc_vpn_providers;
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
  vpn_list_->SetArcVpnProviders(std::move(arc_vpn_providers));
}

void VpnListForwarder::AttachToPrimaryUserProfile() {
  DCHECK(!vpn_list_);
  vpn_list_ = ConnectToVpnList();
  DCHECK(vpn_list_);
  AttachToPrimaryUserExtensionRegistry();
  AttachToPrimaryUserArcAppListPrefs();
}

void VpnListForwarder::AttachToPrimaryUserExtensionRegistry() {
  DCHECK(!extension_registry_);
  extension_registry_ =
      extensions::ExtensionRegistry::Get(GetProfileForPrimaryUser());
  extension_registry_->AddObserver(this);

  UpdateVPNProviders();
}

void VpnListForwarder::AttachToPrimaryUserArcAppListPrefs() {
  DCHECK(arc_app_list_prefs_);
  arc_app_list_prefs_ = ArcAppListPrefs::Get(GetProfileForPrimaryUser());
  if (arc_app_list_prefs_) {
    arc_app_list_prefs_->AddObserver(this);
  }
}
