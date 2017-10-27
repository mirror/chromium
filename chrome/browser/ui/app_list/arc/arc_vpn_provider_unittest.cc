// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_vpn_provider_manager.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_test.h"
#include "components/arc/test/fake_app_instance.h"

constexpr char kVpnAppName[] = "vpn.app.name";
constexpr char kVpnAppNameUpdate[] = "vpn.app.name.update";
constexpr char kVpnAppActivity[] = "vpn.app.activity";
constexpr char kVpnPackageName[] = "vpn.app.package.name";
constexpr char kNonVpnAppName[] = "non.vpn.app.name";
constexpr char kNonVpnAppActivity[] = "non.vpn.app.activity";
constexpr char kNonVpnPackageName[] = "non.vpn.app.package.name";

class ArcVpnObserver : public ArcAppListPrefs::Observer,
                       public app_list::ArcVpnProviderManager::Observer {
 public:
  ArcVpnObserver() = default;
  ~ArcVpnObserver() override = default;

  int getArcVpnProviderUpdateCount(const std::string& package_name) {
    return arc_vpn_provider_counter_[package_name];
  }

  // ArcAppListPrefs::Observer::
  void OnPackageRemoved(const std::string& pacakge_name,
                        bool uninstall) override {
    arc_vpn_provider_counter_.erase(pacakge_name);
  }

  // ArcVpnProviderManager::Observer::
  void OnArcVpnProvidersRefreshed(
      std::vector<
          std::unique_ptr<app_list::ArcVpnProviderManager::ArcVpnProvider>>&
          arc_vpn_providers) override {
    for (const auto& arc_vpn_provider : arc_vpn_providers) {
      arc_vpn_provider_counter_[arc_vpn_provider->package_name] = 1;
    }
  }

  void OnArcVpnProviderUpdated(app_list::ArcVpnProviderManager::ArcVpnProvider*
                                   arc_vpn_provider) override {
    arc_vpn_provider_counter_[arc_vpn_provider->package_name] += 1;
  }

  std::map<std::string, int>& arc_vpn_provider_counter() {
    return arc_vpn_provider_counter_;
  }

 private:
  std::map<std::string, int> arc_vpn_provider_counter_;
};

class ArcVpnProviderTest : public AppListTestBase {
 public:
  ArcVpnProviderTest() = default;
  ~ArcVpnProviderTest() override = default;

  // AppListTestBase:
  void SetUp() override {
    AppListTestBase::SetUp();
    arc_test_.SetUp(profile());
    ArcAppListPrefs* arc_app_list_prefs = ArcAppListPrefs::Get(profile());
    arc_app_list_prefs->AddObserver(&arc_vpn_observer_);
    arc_app_list_prefs->arc_vpn_provider_manager()->AddObserver(
        &arc_vpn_observer_);
  }

  void TearDown() override {
    ArcAppListPrefs* arc_app_list_prefs = ArcAppListPrefs::Get(profile());
    arc_app_list_prefs->RemoveObserver(&arc_vpn_observer_);
    arc_app_list_prefs->arc_vpn_provider_manager()->RemoveObserver(
        &arc_vpn_observer_);
    arc_test_.TearDown();
    AppListTestBase::TearDown();
  }

  void AddArcApp(const std::string& app_name,
                 const std::string& package_name,
                 const std::string& activity) {
    arc::mojom::AppInfo app_info;
    app_info.name = app_name;
    app_info.package_name = package_name;
    app_info.activity = activity;

    std::vector<arc::mojom::AppInfo> apps;
    apps.push_back(app_info);
    app_instance()->SendPackageAppListRefreshed(package_name, apps);
  }

  void AddArcPackage(const std::string& package_name, bool vpn_provider) {
    arc::mojom::ArcPackageInfo package;
    package.package_name = package_name;
    package.package_version = 0;
    package.last_backup_android_id = 0;
    package.last_backup_time = 0;
    package.sync = false;
    package.vpn_provider = vpn_provider;
    app_instance()->SendPackageAdded(package);
  }

  void RemovePackage(const std::string& package_name) {
    app_instance()->UninstallPackage(package_name);
  }

  ArcAppTest& arc_test() { return arc_test_; }

  arc::FakeAppInstance* app_instance() { return arc_test_.app_instance(); }

  ArcVpnObserver& arc_vpn_observer() { return arc_vpn_observer_; }

 private:
  ArcAppTest arc_test_;
  ArcVpnObserver arc_vpn_observer_;
};

TEST_F(ArcVpnProviderTest, ArcVpnProviderUpdateCount) {
  // Starts with no arc vpn provider.
  app_instance()->SendRefreshAppList(std::vector<arc::mojom::AppInfo>());
  app_instance()->SendRefreshPackageList(
      std::vector<arc::mojom::ArcPackageInfo>());

  // Arc Vpn Observer should observe Arc Vpn app installation.
  AddArcApp(kVpnAppName, kVpnPackageName, kVpnAppActivity);
  AddArcPackage(kVpnPackageName, true);

  EXPECT_EQ(arc_vpn_observer().arc_vpn_provider_counter().size(), 1u);
  EXPECT_EQ(arc_vpn_observer().getArcVpnProviderUpdateCount(kVpnPackageName),
            1);

  // Arc Vpn Observer ignores non Arc Vpn app installation.
  AddArcApp(kNonVpnAppName, kNonVpnPackageName, kNonVpnAppActivity);
  AddArcPackage(kNonVpnPackageName, false);
  EXPECT_EQ(arc_vpn_observer().arc_vpn_provider_counter().size(), 1u);

  // Arc Vpn Observer observes Arc Vpn app launch time and app name updates.
  std::string vpnAppId =
      ArcAppListPrefs::GetAppId(kVpnPackageName, kVpnAppActivity);
  arc_test().arc_app_list_prefs()->SetLastLaunchTime(vpnAppId);
  EXPECT_EQ(arc_vpn_observer().getArcVpnProviderUpdateCount(kVpnPackageName),
            2);
  AddArcApp(kVpnAppNameUpdate, kVpnPackageName, kVpnAppActivity);
  EXPECT_EQ(arc_vpn_observer().getArcVpnProviderUpdateCount(kVpnPackageName),
            3);

  // Arc Vpn Observer should observe Arc Vpn app uninstallation.
  RemovePackage(kVpnPackageName);
  EXPECT_EQ(arc_vpn_observer().arc_vpn_provider_counter().size(), 0u);
}
