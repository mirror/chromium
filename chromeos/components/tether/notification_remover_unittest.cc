// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/notification_remover.h"

#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/components/tether/fake_host_scan_cache.h"
#include "chromeos/components/tether/fake_notification_presenter.h"
#include "chromeos/components/tether/host_scan_test_util.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_state_test.h"
#include "components/cryptauth/remote_device.h"
#include "third_party/cros_system_api/dbus/shill/dbus-constants.h"

namespace chromeos {

namespace tether {

class NotificationRemoverTest : public NetworkStateTest {
 protected:
  NotificationRemoverTest()
      : NetworkStateTest(),
        test_entries_(host_scan_test_util::CreateTestEntries()) {}

  void SetUp() override {
    DBusThreadManager::Initialize();
    NetworkStateTest::SetUp();

    notification_presenter_ = base::MakeUnique<FakeNotificationPresenter>();
    host_scan_cache_ = base::MakeUnique<FakeHostScanCache>();

    notification_remover_ = base::MakeUnique<NotificationRemover>(
        host_scan_cache_.get(), network_state_handler(),
        notification_presenter_.get());
  }

  void TearDown() override {
    notification_remover_.reset(nullptr);

    NetworkStateTest::TearDown();
    DBusThreadManager::Shutdown();
  }

  void NotifyPotentialHotspotNearby() {
    cryptauth::RemoteDevice remote_device;
    notification_presenter_->NotifyPotentialHotspotNearby(
        remote_device, 100 /* signal_strength */);
    EXPECT_EQ(FakeNotificationPresenter::PotentialHotspotNotificationState::
                  SINGLE_HOTSPOT_NEARBY_SHOWN,
              notification_presenter_->potential_hotspot_state());
  }

  void AddWifiNetwork() {
    std::stringstream ss;
    ss << "{"
       << "  \"GUID\": \"wifiNetworkGuid\","
       << "  \"Type\": \"" << shill::kTypeWifi << "\","
       << "  \"State\": \"" << shill::kStateOnline << "\""
       << "}";

    ConfigureService(ss.str());
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  const std::unordered_map<std::string, HostScanCacheEntry> test_entries_;

  std::unique_ptr<FakeNotificationPresenter> notification_presenter_;
  std::unique_ptr<FakeHostScanCache> host_scan_cache_;
  std::unique_ptr<NotificationRemover> notification_remover_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationRemoverTest);
};

TEST_F(NotificationRemoverTest, TestCacheBecameEmpty) {
  NotifyPotentialHotspotNearby();

  host_scan_cache_->SetHostScanResult(
      test_entries_.at(host_scan_test_util::kTetherGuid0));
  EXPECT_FALSE(host_scan_cache_->empty());

  host_scan_cache_->RemoveHostScanResult(host_scan_test_util::kTetherGuid0);
  EXPECT_TRUE(host_scan_cache_->empty());
  EXPECT_EQ(FakeNotificationPresenter::PotentialHotspotNotificationState::
                NO_HOTSPOT_NOTIFICATION_SHOWN,
            notification_presenter_->potential_hotspot_state());
}

TEST_F(NotificationRemoverTest, TestConnectToWifiNetwork) {
  NotifyPotentialHotspotNearby();

  AddWifiNetwork();
  EXPECT_EQ(FakeNotificationPresenter::PotentialHotspotNotificationState::
                NO_HOTSPOT_NOTIFICATION_SHOWN,
            notification_presenter_->potential_hotspot_state());
}

}  // namespace tether

}  // namespace chromeos
