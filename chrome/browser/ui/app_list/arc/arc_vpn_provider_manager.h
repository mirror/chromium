// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_VPN_PROVIDER_MANAGER_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_VPN_PROVIDER_MANAGER_H_

// Helper class to create VPN provider specific events to observers. Owned by
// ArcAppListPrefs.

#include <memory>
#include <string>

#include "base/observer_list.h"

class ArcAppListPrefs;

namespace app_list {

class ArcVpnProviderManager {
 public:
  struct ArcVpnProvider {
    ArcVpnProvider(const std::string app_name,
                   const std::string package_name,
                   const std::string app_id,
                   base::Time& last_launch_time);
    ~ArcVpnProvider() {}

    std::string app_name;
    std::string package_name;
    std::string app_id;
    base::Time last_launch_time;
  };

  class Observer {
   public:
    // Notifies initial refresh of Arc VPN providers.
    virtual void OnArcVpnProvidersRefreshed(
        std::vector<std::unique_ptr<ArcVpnProvider>>& arc_vpn_providers) {}
    // Notifies update for an Arc VPN provider. Update includes newly
    // installation, name update, launch time update.
    virtual void OnArcVpnProviderUpdated(ArcVpnProvider* arc_vpn_provider) {}
  };

  explicit ArcVpnProviderManager(ArcAppListPrefs* arc_app_list_prefs);
  ~ArcVpnProviderManager();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  std::vector<std::unique_ptr<ArcVpnProvider>> getArcVpnProviders();

 private:
  friend class ::ArcAppListPrefs;

  void UpdateAppName(const std::string& app_id);
  void UpdatePackageInstalled(const std::string& package_name);
  void UpdatePackageListInitialRefreshed();
  void UpdateLastLauchTime(const std::string& app_id);
  void NotifyArcVpnProviderUpdate(const std::string& app_id);

  ArcAppListPrefs* const arc_app_list_prefs_;

  // List of observers.
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(ArcVpnProviderManager);
};

}  // namespace app_list

#endif  //  CHROME_BROWSER_UI_APP_LIST_ARC_ARC_VPN_PROVIDER_MANAGER_H_
