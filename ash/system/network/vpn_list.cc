// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/vpn_list.h"

#include <utility>

#include "base/logging.h"

namespace ash {

VPNProvider::VPNProvider() : provider_type(VPNProvider::OPN_VPN) {}

VPNProvider::VPNProvider(const std::string& extension_id,
                         const std::string& third_party_provider_name)
    : provider_type(VPNProvider::THIRD_PARTY_VPN),
      app_id(extension_id),
      provider_name(third_party_provider_name) {
  DCHECK(!extension_id.empty());
  DCHECK(!third_party_provider_name.empty());
}

VPNProvider::VPNProvider(const std::string& package_name,
                         const std::string& app_name,
                         const std::string& app_id,
                         const base::Time& last_launch_time)
    : provider_type(VPNProvider::ARC_VPN),
      app_id(app_id),
      provider_name(app_name),
      package_name(package_name),
      last_launch_time(last_launch_time) {
  DCHECK(!app_id.empty());
  DCHECK(!provider_name.empty());
  DCHECK(!package_name.empty());
  DCHECK(!last_launch_time.is_null());
}

VPNProvider::VPNProvider(const VPNProvider& other) {
  provider_type = other.provider_type;
  app_id = other.app_id;
  provider_name = other.provider_name;
  package_name = other.package_name;
  last_launch_time = other.last_launch_time;
}

bool VPNProvider::operator==(const VPNProvider& other) const {
  return provider_type == other.provider_type && app_id == other.app_id &&
         provider_name == other.provider_name &&
         package_name == other.package_name;
}

VpnList::Observer::~Observer() {}

VpnList::VpnList() {
  AddBuiltInProvider();
}

VpnList::~VpnList() {}

bool VpnList::HaveThirdPartyOrArcVPNProviders() const {
  for (const VPNProvider& provider : vpn_providers_) {
    if (provider.provider_type == VPNProvider::THIRD_PARTY_VPN)
      return true;
  }
  return arc_vpn_providers_.size() > 0;
}

void VpnList::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void VpnList::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void VpnList::BindRequest(mojom::VpnListRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void VpnList::SetThirdPartyVpnProviders(
    std::vector<mojom::ThirdPartyVpnProviderPtr> providers) {
  vpn_providers_.clear();
  vpn_providers_.reserve(providers.size() + 1);
  // Add the OpenVPN provider.
  AddBuiltInProvider();
  // Append the extension-backed providers.
  for (const auto& provider : providers) {
    vpn_providers_.push_back(
        VPNProvider(provider->extension_id, provider->name));
  }
  NotifyObservers();
}

void VpnList::SetArcVpnProviders(
    std::vector<mojom::ArcVpnProviderPtr> arc_providers) {
  arc_vpn_providers_.clear();
  arc_vpn_providers_.reserve(arc_providers.size());

  for (const auto& arc_provider : arc_providers) {
    arc_vpn_providers_.push_back(
        VPNProvider(arc_provider->package_name, arc_provider->app_name,
                    arc_provider->app_id, arc_provider->last_launch_time));
  }
  NotifyObservers();
}

void VpnList::AddOrUpdateArcVPNProvider(mojom::ArcVpnProviderPtr arc_provider) {
  bool provider_found = false;
  for (auto& arc_vpn_provider : arc_vpn_providers_) {
    if (arc_vpn_provider.package_name == arc_provider->package_name) {
      arc_vpn_provider.provider_name = arc_provider->app_name;
      arc_vpn_provider.app_id = arc_provider->app_id;
      arc_vpn_provider.last_launch_time = arc_provider->last_launch_time;
      provider_found = true;
      break;
    }
  }
  // This is a newly install ArcVPNProvider.
  if (!provider_found) {
    arc_vpn_providers_.push_back(
        VPNProvider(arc_provider->package_name, arc_provider->app_name,
                    arc_provider->app_id, arc_provider->last_launch_time));
  }
  NotifyObservers();
}

void VpnList::RemoveArcVPNProvider(const std::string& package_name) {
  bool provider_found_and_removed = false;
  for (auto arc_vpn_provider = arc_vpn_providers_.begin();
       arc_vpn_provider != arc_vpn_providers_.end(); arc_vpn_provider++) {
    if (arc_vpn_provider->package_name == package_name) {
      arc_vpn_providers_.erase(arc_vpn_provider);
      provider_found_and_removed = true;
      break;
    }
  }
  if (provider_found_and_removed)
    NotifyObservers();
}

void VpnList::NotifyObservers() {
  for (auto& observer : observer_list_)
    observer.OnVPNProvidersChanged();
}

void VpnList::AddBuiltInProvider() {
  // The VPNProvider() constructor generates the built-in provider and has no
  // extension ID.
  vpn_providers_.push_back(VPNProvider());
}

}  // namespace ash
