// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/persistent_host_scan_cache.h"

#include <memory>
#include <unordered_map>

#include "base/memory/ptr_util.h"
#include "chromeos/components/tether/fake_host_scan_cache.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

const char kTetherGuid0[] = "kTetherGuid0";
const char kTetherGuid1[] = "kTetherGuid1";
const char kTetherGuid2[] = "kTetherGuid2";
const char kTetherGuid3[] = "kTetherGuid3";

const char kTetherDeviceName0[] = "kDeviceName0";
const char kTetherDeviceName1[] = "kDeviceName1";
const char kTetherDeviceName2[] = "kDeviceName2";
const char kTetherDeviceName3[] = "kDeviceName3";

const char kTetherCarrier0[] = "kTetherCarrier0";
const char kTetherCarrier1[] = "kTetherCarrier1";
const char kTetherCarrier2[] = "kTetherCarrier2";
const char kTetherCarrier3[] = "kTetherCarrier3";

const int kTetherBatteryPercentage0 = 20;
const int kTetherBatteryPercentage1 = 40;
const int kTetherBatteryPercentage2 = 60;
const int kTetherBatteryPercentage3 = 80;

const int kTetherSignalStrength0 = 25;
const int kTetherSignalStrength1 = 50;
const int kTetherSignalStrength2 = 75;
const int kTetherSignalStrength3 = 100;

const bool kTetherSetupRequired0 = true;
const bool kTetherSetupRequired1 = false;
const bool kTetherSetupRequired2 = true;
const bool kTetherSetupRequired3 = false;

std::unordered_map<std::string, HostScanCacheEntry> CreateTestEntries() {
  std::unordered_map<std::string, HostScanCacheEntry> entries;

  entries.emplace(kTetherGuid0,
                  *HostScanCacheEntry::Builder()
                       .SetTetherNetworkGuid(kTetherGuid0)
                       .SetDeviceName(kTetherDeviceName0)
                       .SetCarrier(kTetherCarrier0)
                       .SetBatteryPercentage(kTetherBatteryPercentage0)
                       .SetSignalStrength(kTetherSignalStrength0)
                       .SetSetupRequired(kTetherSetupRequired0)
                       .Build());

  entries.emplace(kTetherGuid1,
                  *HostScanCacheEntry::Builder()
                       .SetTetherNetworkGuid(kTetherGuid1)
                       .SetDeviceName(kTetherDeviceName1)
                       .SetCarrier(kTetherCarrier1)
                       .SetBatteryPercentage(kTetherBatteryPercentage1)
                       .SetSignalStrength(kTetherSignalStrength1)
                       .SetSetupRequired(kTetherSetupRequired1)
                       .Build());

  entries.emplace(kTetherGuid2,
                  *HostScanCacheEntry::Builder()
                       .SetTetherNetworkGuid(kTetherGuid2)
                       .SetDeviceName(kTetherDeviceName2)
                       .SetCarrier(kTetherCarrier2)
                       .SetBatteryPercentage(kTetherBatteryPercentage2)
                       .SetSignalStrength(kTetherSignalStrength2)
                       .SetSetupRequired(kTetherSetupRequired2)
                       .Build());

  entries.emplace(kTetherGuid3,
                  *HostScanCacheEntry::Builder()
                       .SetTetherNetworkGuid(kTetherGuid3)
                       .SetDeviceName(kTetherDeviceName3)
                       .SetCarrier(kTetherCarrier3)
                       .SetBatteryPercentage(kTetherBatteryPercentage3)
                       .SetSignalStrength(kTetherSignalStrength3)
                       .SetSetupRequired(kTetherSetupRequired3)
                       .Build());

  return entries;
}

}  // namespace

class PersistentHostScanCacheTest : public testing::Test {
 protected:
  PersistentHostScanCacheTest() : test_entries_(CreateTestEntries()) {}

  void SetUp() override {
    test_pref_service_ = base::MakeUnique<TestingPrefServiceSimple>();
    PersistentHostScanCache::RegisterPrefs(test_pref_service_->registry());

    host_scan_cache_ =
        base::MakeUnique<PersistentHostScanCache>(test_pref_service_.get());
    expected_cache_ = base::MakeUnique<FakeHostScanCache>();
  }

  void SetHostScanResult(const HostScanCacheEntry& entry) {
    host_scan_cache_->SetHostScanResult(entry);
    expected_cache_->SetHostScanResult(entry);
  }

  void RemoveHostScanResult(const std::string& tether_network_guid) {
    host_scan_cache_->RemoveHostScanResult(tether_network_guid);
    expected_cache_->RemoveHostScanResult(tether_network_guid);
  }

  void VerifyPersistentCacheMatchesInMemoryCache(size_t expected_size) {
    std::unordered_map<std::string, HostScanCacheEntry> entries =
        host_scan_cache_->GetStoredCacheEntries();
    EXPECT_EQ(expected_size, entries.size());
    EXPECT_EQ(expected_size, expected_cache_->cache().size());
    EXPECT_EQ(expected_cache_->cache(), entries);
  }

  const std::unordered_map<std::string, HostScanCacheEntry> test_entries_;

  std::unique_ptr<TestingPrefServiceSimple> test_pref_service_;
  std::unique_ptr<FakeHostScanCache> expected_cache_;

  std::unique_ptr<PersistentHostScanCache> host_scan_cache_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PersistentHostScanCacheTest);
};

TEST_F(PersistentHostScanCacheTest, TestSetAndRemove) {
  SetHostScanResult(test_entries_.at(kTetherGuid0));
  VerifyPersistentCacheMatchesInMemoryCache(1u /* expected_size */);

  SetHostScanResult(test_entries_.at(kTetherGuid1));
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);

  SetHostScanResult(test_entries_.at(kTetherGuid2));
  VerifyPersistentCacheMatchesInMemoryCache(3u /* expected_size */);

  SetHostScanResult(test_entries_.at(kTetherGuid3));
  VerifyPersistentCacheMatchesInMemoryCache(4u /* expected_size */);

  RemoveHostScanResult(kTetherGuid0);
  VerifyPersistentCacheMatchesInMemoryCache(3u /* expected_size */);

  RemoveHostScanResult(kTetherGuid1);
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);

  RemoveHostScanResult(kTetherGuid2);
  VerifyPersistentCacheMatchesInMemoryCache(1u /* expected_size */);

  RemoveHostScanResult(kTetherGuid3);
  VerifyPersistentCacheMatchesInMemoryCache(0u /* expected_size */);
}

TEST_F(PersistentHostScanCacheTest, TestUpdate) {
  SetHostScanResult(test_entries_.at(kTetherGuid0));
  VerifyPersistentCacheMatchesInMemoryCache(1u /* expected_size */);
  EXPECT_EQ(kTetherSetupRequired0,
            host_scan_cache_->DoesHostRequireSetup(kTetherGuid0));

  // Update existing entry, including changing the "setup required" field.
  SetHostScanResult(*HostScanCacheEntry::Builder()
                         .SetTetherNetworkGuid(kTetherGuid0)
                         .SetDeviceName(kTetherDeviceName0)
                         .SetCarrier(kTetherCarrier1)
                         .SetBatteryPercentage(kTetherBatteryPercentage1)
                         .SetSignalStrength(kTetherSignalStrength1)
                         .SetSetupRequired(kTetherSetupRequired1)
                         .Build());
  VerifyPersistentCacheMatchesInMemoryCache(1u /* expected_size */);
  EXPECT_EQ(kTetherSetupRequired1,
            host_scan_cache_->DoesHostRequireSetup(kTetherGuid0));
}

TEST_F(PersistentHostScanCacheTest, TestStoredPersistently) {
  SetHostScanResult(test_entries_.at(kTetherGuid0));
  VerifyPersistentCacheMatchesInMemoryCache(1u /* expected_size */);

  SetHostScanResult(test_entries_.at(kTetherGuid1));
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);

  // Now, delete the existing PersistentHostScanCache object. All of its
  // in-memory state will be cleaned up, but it should have stored the scanned
  // data persistently.
  host_scan_cache_.reset();

  // Create a new object.
  host_scan_cache_ =
      base::MakeUnique<PersistentHostScanCache>(test_pref_service_.get());

  // The new object should still access the stored scanned data.
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);

  // Make some changes - update one existing result, add a new one, and remove
  // an old one.
  SetHostScanResult(*HostScanCacheEntry::Builder()
                         .SetTetherNetworkGuid(kTetherGuid0)
                         .SetDeviceName(kTetherDeviceName0)
                         .SetCarrier(kTetherCarrier1)
                         .SetBatteryPercentage(kTetherBatteryPercentage1)
                         .SetSignalStrength(kTetherSignalStrength1)
                         .SetSetupRequired(kTetherSetupRequired1)
                         .Build());
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);

  SetHostScanResult(test_entries_.at(kTetherGuid2));
  VerifyPersistentCacheMatchesInMemoryCache(3u /* expected_size */);

  RemoveHostScanResult(kTetherGuid1);
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);

  // Delete the current PersistentHostScanCache and create another new one. It
  // should still contain the same data.
  host_scan_cache_.reset(new PersistentHostScanCache(test_pref_service_.get()));
  VerifyPersistentCacheMatchesInMemoryCache(2u /* expected_size */);
}

}  // namespace tether

}  // namespace chromeos
