// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/master_host_scan_cache.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/timer/mock_timer.h"
#include "chromeos/components/tether/device_id_tether_network_guid_map.h"
#include "chromeos/components/tether/fake_active_host.h"
#include "chromeos/components/tether/fake_host_scan_cache.h"
#include "chromeos/components/tether/host_scan_test_util.h"
#include "chromeos/components/tether/persistent_host_scan_cache.h"
#include "chromeos/components/tether/timer_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

class FakePersistentHostScanCache : public FakeHostScanCache,
                                    public PersistentHostScanCache {
 public:
  FakePersistentHostScanCache() {}
  ~FakePersistentHostScanCache() override {}

  // PersistentHostScanCache:
  std::unordered_map<std::string, HostScanCacheEntry> GetStoredCacheEntries()
      override {
    return cache();
  }
};

// MockTimer which invokes a callback in its destructor.
class ExtendedMockTimer : public base::MockTimer {
 public:
  explicit ExtendedMockTimer(const base::Closure& destructor_callback)
      : base::MockTimer(true /* retain_user_task */, false /* is_repeating */),
        destructor_callback_(destructor_callback) {}

  ~ExtendedMockTimer() override { destructor_callback_.Run(); }

 private:
  base::Closure destructor_callback_;
};

class TestTimerFactory : public TimerFactory {
 public:
  TestTimerFactory() {}
  ~TestTimerFactory() override {}

  std::unordered_map<std::string, ExtendedMockTimer*>&
  tether_network_guid_to_timer_map() {
    return tether_network_guid_to_timer_map_;
  }

  void set_tether_network_guid_for_next_timer(
      const std::string& tether_network_guid_for_next_timer) {
    tether_network_guid_for_next_timer_ = tether_network_guid_for_next_timer;
  }

  // TimerFactory:
  std::unique_ptr<base::Timer> CreateOneShotTimer() override {
    EXPECT_FALSE(tether_network_guid_for_next_timer_.empty());
    ExtendedMockTimer* mock_timer = new ExtendedMockTimer(base::Bind(
        &TestTimerFactory::OnActiveTimerDestructor, base::Unretained(this),
        tether_network_guid_for_next_timer_));
    tether_network_guid_to_timer_map_[tether_network_guid_for_next_timer_] =
        mock_timer;
    return base::WrapUnique(mock_timer);
  }

 private:
  void OnActiveTimerDestructor(const std::string& tether_network_guid) {
    tether_network_guid_to_timer_map_.erase(
        tether_network_guid_to_timer_map_.find(tether_network_guid));
  }

  std::string tether_network_guid_for_next_timer_;
  std::unordered_map<std::string, ExtendedMockTimer*>
      tether_network_guid_to_timer_map_;
};

}  // namespace

// TODO(khorimoto): The test uses a FakeHostScanCache to keep an in-memory
// cache of expected values. This has the potential to be confusing, since this
// is the test for MasterHostScanCache. Clean this up to avoid using
// FakeHostScanCache if possible.
class MasterHostScanCacheTest : public testing::Test {
 protected:
  MasterHostScanCacheTest()
      : test_entries_(host_scan_test_util::CreateTestEntries()) {}

  void SetUp() override {
    test_timer_factory_ = new TestTimerFactory();
    fake_active_host_ = base::MakeUnique<FakeActiveHost>();
    fake_network_host_scan_cache_ = base::MakeUnique<FakeHostScanCache>();
    fake_persistent_host_scan_cache_ =
        base::WrapUnique(new FakePersistentHostScanCache());

    host_scan_cache_ = base::MakeUnique<MasterHostScanCache>(
        fake_active_host_.get(), fake_network_host_scan_cache_.get(),
        fake_persistent_host_scan_cache_.get());
    host_scan_cache_->SetTimerFactoryForTest(
        base::WrapUnique(test_timer_factory_));

    // To track what is expected to be contained in the cache, maintain a
    // FakeHostScanCache in memory and update it alongside |host_scan_cache_|.
    // Use a std::vector to track which device IDs correspond to devices whose
    // Tether networks' HasConnectedToHost fields are expected to be set.
    expected_cache_ = base::MakeUnique<FakeHostScanCache>();

    device_id_tether_network_guid_map_ =
        base::MakeUnique<DeviceIdTetherNetworkGuidMap>();
  }

  void FireTimer(const std::string& tether_network_guid) {
    ExtendedMockTimer* timer =
        test_timer_factory_
            ->tether_network_guid_to_timer_map()[tether_network_guid];
    ASSERT_TRUE(timer);
    timer->Fire();

    // If the device whose correlated timer has fired is not the active host, it
    // is expected to be removed from the cache.
    EXPECT_EQ(fake_active_host_->GetTetherNetworkGuid(),
              expected_cache_->active_host_tether_network_guid());
    if (fake_active_host_->GetTetherNetworkGuid() != tether_network_guid) {
      expected_cache_->RemoveHostScanResult(tether_network_guid);
    }
  }

  void SetActiveHost(const std::string& tether_network_guid) {
    fake_active_host_->SetActiveHostConnected(
        device_id_tether_network_guid_map_->GetDeviceIdForTetherNetworkGuid(
            tether_network_guid),
        tether_network_guid, "wifiNetworkGuid");
    expected_cache_->set_active_host_tether_network_guid(tether_network_guid);
  }

  void SetHostScanResult(const HostScanCacheEntry& entry) {
    test_timer_factory_->set_tether_network_guid_for_next_timer(
        entry.tether_network_guid);
    host_scan_cache_->SetHostScanResult(entry);
    expected_cache_->SetHostScanResult(entry);
  }

  void RemoveHostScanResult(const std::string& tether_network_guid) {
    host_scan_cache_->RemoveHostScanResult(tether_network_guid);
    expected_cache_->RemoveHostScanResult(tether_network_guid);
  }

  void ClearCacheExceptForActiveHost() {
    host_scan_cache_->ClearCacheExceptForActiveHost();
    expected_cache_->ClearCacheExceptForActiveHost();
  }

  // Verifies that the information present in |expected_cache_| mirrors what
  // |host_scan_cache_| has stored.
  void VerifyCacheContainsExpectedContents(size_t expected_size) {
    EXPECT_EQ(expected_size, expected_cache_->size());
    EXPECT_EQ(expected_size, fake_network_host_scan_cache_->size());
    EXPECT_EQ(expected_size, fake_persistent_host_scan_cache_->size());

    for (auto& it : expected_cache_->cache()) {
      const std::string tether_network_guid = it.first;
      const HostScanCacheEntry& expected_entry = it.second;

      const HostScanCacheEntry* network_entry =
          fake_network_host_scan_cache_->GetCacheEntry(tether_network_guid);
      ASSERT_TRUE(network_entry);

      const HostScanCacheEntry* persistent_entry =
          fake_persistent_host_scan_cache_->GetCacheEntry(tether_network_guid);
      ASSERT_TRUE(persistent_entry);

      EXPECT_EQ(expected_entry.device_name, network_entry->device_name);
      EXPECT_EQ(expected_entry.device_name, persistent_entry->device_name);
      EXPECT_EQ(expected_entry.carrier, network_entry->carrier);
      EXPECT_EQ(expected_entry.carrier, persistent_entry->carrier);
      EXPECT_EQ(expected_entry.battery_percentage,
                network_entry->battery_percentage);
      EXPECT_EQ(expected_entry.battery_percentage,
                persistent_entry->battery_percentage);
      EXPECT_EQ(expected_entry.signal_strength, network_entry->signal_strength);
      EXPECT_EQ(expected_entry.signal_strength,
                persistent_entry->signal_strength);
      EXPECT_EQ(expected_entry.setup_required, network_entry->setup_required);
      EXPECT_EQ(expected_entry.setup_required,
                persistent_entry->setup_required);

      // Ensure that each entry has an actively-running Timer.
      ExtendedMockTimer* timer =
          test_timer_factory_
              ->tether_network_guid_to_timer_map()[tether_network_guid];
      ASSERT_TRUE(timer);
      EXPECT_TRUE(timer->IsRunning());
    }
  }

  const std::unordered_map<std::string, HostScanCacheEntry> test_entries_;

  TestTimerFactory* test_timer_factory_;
  std::unique_ptr<FakeActiveHost> fake_active_host_;
  std::unique_ptr<FakeHostScanCache> fake_network_host_scan_cache_;
  std::unique_ptr<FakePersistentHostScanCache> fake_persistent_host_scan_cache_;

  std::unique_ptr<FakeHostScanCache> expected_cache_;
  // TODO(hansberry): Use a fake for this when a real mapping scheme is created.
  std::unique_ptr<DeviceIdTetherNetworkGuidMap>
      device_id_tether_network_guid_map_;

  std::unique_ptr<MasterHostScanCache> host_scan_cache_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MasterHostScanCacheTest);
};

TEST_F(MasterHostScanCacheTest, TestSetScanResultsAndLetThemExpire) {
  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid0));
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid1));
  VerifyCacheContainsExpectedContents(2u /* expected_size */);

  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid2));
  VerifyCacheContainsExpectedContents(3u /* expected_size */);

  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid3));
  VerifyCacheContainsExpectedContents(4u /* expected_size */);

  FireTimer(host_scan_test_util::kTetherGuid0);
  VerifyCacheContainsExpectedContents(3u /* expected_size */);

  FireTimer(host_scan_test_util::kTetherGuid1);
  VerifyCacheContainsExpectedContents(2u /* expected_size */);

  FireTimer(host_scan_test_util::kTetherGuid2);
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  FireTimer(host_scan_test_util::kTetherGuid3);
  VerifyCacheContainsExpectedContents(0 /* expected_size */);
}

TEST_F(MasterHostScanCacheTest, TestSetScanResultThenUpdateAndRemove) {
  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid0));
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  // Change the fields for tether network with GUID |kTetherGuid0| to the
  // fields corresponding to |kTetherGuid1|.
  SetHostScanResult(
      *HostScanCacheEntry::Builder()
           .SetTetherNetworkGuid(host_scan_test_util::kTetherGuid0)
           .SetDeviceName(host_scan_test_util::kTetherDeviceName0)
           .SetCarrier(host_scan_test_util::kTetherCarrier1)
           .SetBatteryPercentage(host_scan_test_util::kTetherBatteryPercentage1)
           .SetSignalStrength(host_scan_test_util::kTetherSignalStrength1)
           .SetSetupRequired(host_scan_test_util::kTetherSetupRequired1)
           .Build());
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  // Now, remove that result.
  RemoveHostScanResult(host_scan_test_util::kTetherGuid0);
  VerifyCacheContainsExpectedContents(0 /* expected_size */);
}

TEST_F(MasterHostScanCacheTest, TestSetScanResult_SetActiveHost_ThenClear) {
  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid0));
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid1));
  VerifyCacheContainsExpectedContents(2u /* expected_size */);

  SetHostScanResult(test_entries_.at(host_scan_test_util::kTetherGuid2));
  VerifyCacheContainsExpectedContents(3u /* expected_size */);

  // Now, set the active host to be the device 0.
  SetActiveHost(host_scan_test_util::kTetherGuid0);

  // Clear the cache except for the active host.
  ClearCacheExceptForActiveHost();
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  // Attempt to remove the active host. This operation should fail since
  // removing the active host from the cache is not allowed.
  RemoveHostScanResult(host_scan_test_util::kTetherGuid0);
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  // Fire the timer for the active host. Likewise, this should not result in the
  // cache entry being removed.
  FireTimer(host_scan_test_util::kTetherGuid0);
  VerifyCacheContainsExpectedContents(1u /* expected_size */);

  // Now, unset the active host.
  SetActiveHost("");

  // Removing the device should now succeed.
  RemoveHostScanResult(host_scan_test_util::kTetherGuid0);
  EXPECT_TRUE(expected_cache_->empty());
  VerifyCacheContainsExpectedContents(0 /* expected_size */);
}

}  // namespace tether

}  // namespace chromeos
