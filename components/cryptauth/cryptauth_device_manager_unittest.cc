// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_device_manager.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/base64url.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/simple_test_clock.h"
#include "components/cryptauth/fake_cryptauth_gcm_manager.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "components/cryptauth/mock_sync_scheduler.h"
#include "components/cryptauth/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace cryptauth {
namespace {

// The initial "Now" time for testing.
const double kInitialTimeNowSeconds = 20000000;

// A later "Now" time for testing.
const double kLaterTimeNowSeconds = kInitialTimeNowSeconds + 30;

// The timestamp of a last successful sync in seconds.
const double kLastSyncTimeSeconds = kInitialTimeNowSeconds - (60 * 60 * 5);

// Unlock key fields originally stored in the user prefs.
const char kStoredPublicKey[] = "AAPL";
const char kStoredDeviceName[] = "iPhone 6";
const char kStoredBluetoothAddress[] = "12:34:56:78:90:AB";
const bool kStoredUnlockKey = true;
const bool kStoredUnlockable = false;
const bool kStoredMobileHotspotSupported = true;

// ExternalDeviceInfo fields for the synced unlock key.
const char kPublicKey1[] = "GOOG";
const char kDeviceName1[] = "Pixel XL";
const char kBluetoothAddress1[] = "aa:bb:cc:ee:dd:ff";
const bool kUnlockKey1 = true;
const bool kUnlockable1 = false;
const bool kMobileHotspotSupported1 = true;
const char kBeaconSeed1Data[] = "beaconSeed1Data";
const int64_t kBeaconSeed1StartTime = 123456;
const int64_t kBeaconSeed1EndTime = 123457;
const char kBeaconSeed2Data[] = "beaconSeed2Data";
const int64_t kBeaconSeed2StartTime = 234567;
const int64_t kBeaconSeed2EndTime = 234568;
const bool kArcPlusPlus1 = true;
const bool kPixelPhone1 = true;

// ExternalDeviceInfo fields for a non-synced unlockable device.
const char kPublicKey2[] = "MSFT";
const char kDeviceName2[] = "Surface Pro 3";
const bool kUnlockKey2 = false;
const bool kUnlockable2 = true;
const bool kMobileHotspotSupported2 = false;
const char kBeaconSeed3Data[] = "beaconSeed3Data";
const int64_t kBeaconSeed3StartTime = 123456;
const int64_t kBeaconSeed3EndTime = 123457;
const char kBeaconSeed4Data[] = "beaconSeed4Data";
const int64_t kBeaconSeed4StartTime = 234567;
const int64_t kBeaconSeed4EndTime = 234568;
const bool kArcPlusPlus2 = false;
const bool kPixelPhone2 = false;

// Validates that |devices| is equal to |expected_devices|.
void ExpectSyncedDevicesAreEqual(
    const std::vector<ExternalDeviceInfo>& expected_devices,
    const std::vector<ExternalDeviceInfo>& devices) {
  ASSERT_EQ(expected_devices.size(), devices.size());
  for (size_t i = 0; i < devices.size(); ++i) {
    SCOPED_TRACE(
        base::StringPrintf("Compare protos at index=%d", static_cast<int>(i)));
    const auto& expected_device = expected_devices[i];
    const auto& device = devices.at(i);
    EXPECT_TRUE(expected_device.has_public_key());
    EXPECT_TRUE(device.has_public_key());
    EXPECT_EQ(expected_device.public_key(),
              device.public_key());

    EXPECT_EQ(expected_device.has_friendly_device_name(),
              device.has_friendly_device_name());
    EXPECT_EQ(expected_device.friendly_device_name(),
              device.friendly_device_name());

    EXPECT_EQ(expected_device.has_bluetooth_address(),
              device.has_bluetooth_address());
    EXPECT_EQ(expected_device.bluetooth_address(),
              device.bluetooth_address());

    EXPECT_EQ(expected_device.has_unlock_key(),
              device.has_unlock_key());
    EXPECT_EQ(expected_device.unlock_key(),
              device.unlock_key());

    EXPECT_EQ(expected_device.has_unlockable(),
              device.has_unlockable());
    EXPECT_EQ(expected_device.unlockable(),
              device.unlockable());

    EXPECT_EQ(expected_device.has_last_update_time_millis(),
              device.has_last_update_time_millis());
    EXPECT_EQ(expected_device.last_update_time_millis(),
              device.last_update_time_millis());

    EXPECT_EQ(expected_device.has_mobile_hotspot_supported(),
              device.has_mobile_hotspot_supported());
    EXPECT_EQ(expected_device.mobile_hotspot_supported(),
              device.mobile_hotspot_supported());

    EXPECT_EQ(expected_device.has_device_type(),
              device.has_device_type());
    EXPECT_EQ(expected_device.device_type(),
              device.device_type());

    ASSERT_EQ(expected_device.beacon_seeds_size(),
              device.beacon_seeds_size());
    for (int i = 0; i < expected_device.beacon_seeds_size(); i++) {
      const BeaconSeed expected_seed =
          expected_device.beacon_seeds(i);
      const BeaconSeed seed = device.beacon_seeds(i);
      EXPECT_TRUE(expected_seed.has_data());
      EXPECT_TRUE(seed.has_data());
      EXPECT_EQ(expected_seed.data(), seed.data());

      EXPECT_TRUE(expected_seed.has_start_time_millis());
      EXPECT_TRUE(seed.has_start_time_millis());
      EXPECT_EQ(expected_seed.start_time_millis(), seed.start_time_millis());

      EXPECT_TRUE(expected_seed.has_end_time_millis());
      EXPECT_TRUE(seed.has_end_time_millis());
      EXPECT_EQ(expected_seed.end_time_millis(), seed.end_time_millis());
    }
  }
}

// Validates that |devices| and the corresponding preferences stored by
// |pref_service| are equal to |expected_devices|.
void ExpectSyncedDevicesAndPrefAreEqual(
    const std::vector<ExternalDeviceInfo>& expected_devices,
    const std::vector<ExternalDeviceInfo>& devices,
    const PrefService& pref_service) {
  ExpectSyncedDevicesAreEqual(expected_devices, devices);

  const base::ListValue* synced_devices_pref =
      pref_service.GetList(prefs::kCryptAuthDeviceSyncUnlockKeys);
  ASSERT_EQ(expected_devices.size(), synced_devices_pref->GetSize());
  for (size_t i = 0; i < synced_devices_pref->GetSize(); ++i) {
    SCOPED_TRACE(base::StringPrintf("Compare pref dictionary at index=%d",
                                    static_cast<int>(i)));
    const base::DictionaryValue* device_dictionary;
    EXPECT_TRUE(synced_devices_pref->GetDictionary(i, &device_dictionary));

    const auto& expected_device = expected_devices[i];

    std::string public_key_b64, public_key;
    EXPECT_TRUE(
        device_dictionary->GetString("public_key", &public_key_b64));
    EXPECT_TRUE(base::Base64UrlDecode(
        public_key_b64, base::Base64UrlDecodePolicy::REQUIRE_PADDING,
        &public_key));
    EXPECT_TRUE(expected_device.has_public_key());
    EXPECT_EQ(expected_device.public_key(), public_key);

    std::string device_name_b64, device_name;
    if (device_dictionary->GetString("device_name", &device_name_b64)) {
      EXPECT_TRUE(base::Base64UrlDecode(
          device_name_b64, base::Base64UrlDecodePolicy::REQUIRE_PADDING,
          &device_name));
      EXPECT_TRUE(expected_device.has_friendly_device_name());
      EXPECT_EQ(expected_device.friendly_device_name(), device_name);
    } else {
      EXPECT_FALSE(expected_device.has_friendly_device_name());
    }

    std::string bluetooth_address_b64, bluetooth_address;
    if (device_dictionary->GetString("bluetooth_address",
                                         &bluetooth_address_b64)) {
      EXPECT_TRUE(base::Base64UrlDecode(
          bluetooth_address_b64, base::Base64UrlDecodePolicy::REQUIRE_PADDING,
          &bluetooth_address));
      EXPECT_TRUE(expected_device.has_bluetooth_address());
      EXPECT_EQ(expected_device.bluetooth_address(), bluetooth_address);
    } else {
      EXPECT_FALSE(expected_device.has_bluetooth_address());
    }

    bool unlock_key;
    if (device_dictionary->GetBoolean("unlock_key", &unlock_key)) {
      EXPECT_TRUE(expected_device.has_unlock_key());
      EXPECT_EQ(expected_device.unlock_key(), unlock_key);
    } else {
      EXPECT_FALSE(expected_device.has_unlock_key());
    }

    bool unlockable;
    if (device_dictionary->GetBoolean("unlockable", &unlockable)) {
      EXPECT_TRUE(expected_device.has_unlockable());
      EXPECT_EQ(expected_device.unlockable(), unlockable);
    } else {
      EXPECT_FALSE(expected_device.has_unlockable());
    }

    std::string last_update_time_millis_str;
    if (device_dictionary->GetString("last_update_time_millis",
                                         &last_update_time_millis_str)) {
      int64_t last_update_time_millis;
      EXPECT_TRUE(base::StringToInt64(last_update_time_millis_str,
                                      &last_update_time_millis));
      EXPECT_TRUE(expected_device.has_last_update_time_millis());
      EXPECT_EQ(expected_device.last_update_time_millis(),
                last_update_time_millis);
    } else {
      EXPECT_FALSE(expected_device.has_last_update_time_millis());
    }

    bool mobile_hotspot_supported;
    if (device_dictionary->GetBoolean("mobile_hotspot_supported",
                                          &mobile_hotspot_supported)) {
      EXPECT_TRUE(expected_device.has_mobile_hotspot_supported());
      EXPECT_EQ(expected_device.mobile_hotspot_supported(),
                mobile_hotspot_supported);
    } else {
      EXPECT_FALSE(expected_device.has_mobile_hotspot_supported());
    }

    int device_type;
    if (device_dictionary->GetInteger("device_type",
                                          &device_type)) {
      EXPECT_TRUE(expected_device.has_device_type());
      EXPECT_EQ(expected_device.device_type(),
                device_type);
    } else {
      EXPECT_FALSE(expected_device.has_device_type());
    }

    const base::ListValue* beacon_seeds_from_prefs;
    if (device_dictionary->GetList("beacon_seeds",
                                       &beacon_seeds_from_prefs)) {
      ASSERT_EQ(
          (size_t) expected_device.beacon_seeds_size(),
          beacon_seeds_from_prefs->GetSize());
      for (size_t i = 0; i < beacon_seeds_from_prefs->GetSize(); i++) {
        const base::DictionaryValue* seed;
        ASSERT_TRUE(beacon_seeds_from_prefs->GetDictionary(i, &seed));

        std::string data_b64, start_ms, end_ms;
        EXPECT_TRUE(seed->GetString("beacon_seed_data", &data_b64));
        EXPECT_TRUE(seed->GetString("beacon_seed_start_ms", &start_ms));
        EXPECT_TRUE(seed->GetString("beacon_seed_end_ms", &end_ms));

        const BeaconSeed& expected_seed =
            expected_device.beacon_seeds((int) i);

        std::string data;
        EXPECT_TRUE(base::Base64UrlDecode(
            data_b64, base::Base64UrlDecodePolicy::REQUIRE_PADDING, &data));
        EXPECT_TRUE(expected_seed.has_data());
        EXPECT_EQ(expected_seed.data(), data);

        EXPECT_TRUE(expected_seed.has_start_time_millis());
        EXPECT_EQ(expected_seed.start_time_millis(), std::stol(start_ms));

        EXPECT_TRUE(expected_seed.has_end_time_millis());
        EXPECT_EQ(expected_seed.end_time_millis(), std::stol(end_ms));
      }
    } else {
      EXPECT_FALSE(expected_device.beacon_seeds_size());
    }
  }
}

// Harness for testing CryptAuthDeviceManager.
class TestCryptAuthDeviceManager : public CryptAuthDeviceManager {
 public:
  TestCryptAuthDeviceManager(
      base::Clock* clock,
      std::unique_ptr<CryptAuthClientFactory> client_factory,
      CryptAuthGCMManager* gcm_manager,
      PrefService* pref_service)
      : CryptAuthDeviceManager(clock,
                               std::move(client_factory),
                               gcm_manager,
                               pref_service),
        scoped_sync_scheduler_(new NiceMock<MockSyncScheduler>()),
        weak_sync_scheduler_factory_(scoped_sync_scheduler_) {
    SetSyncSchedulerForTest(base::WrapUnique(scoped_sync_scheduler_));
  }

  ~TestCryptAuthDeviceManager() override {}

  base::WeakPtr<MockSyncScheduler> GetSyncScheduler() {
    return weak_sync_scheduler_factory_.GetWeakPtr();
  }

 private:
  // Ownership is passed to |CryptAuthDeviceManager| super class when
  // SetSyncSchedulerForTest() is called.
  MockSyncScheduler* scoped_sync_scheduler_;

  // Stores the pointer of |scoped_sync_scheduler_| after ownership is passed to
  // the super class.
  // This should be safe because the life-time this SyncScheduler will always be
  // within the life of the TestCryptAuthDeviceManager object.
  base::WeakPtrFactory<MockSyncScheduler> weak_sync_scheduler_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestCryptAuthDeviceManager);
};

}  // namespace

class CryptAuthDeviceManagerTest
    : public testing::Test,
      public CryptAuthDeviceManager::Observer,
      public MockCryptAuthClientFactory::Observer {
 protected:
  CryptAuthDeviceManagerTest()
      : client_factory_(new MockCryptAuthClientFactory(
            MockCryptAuthClientFactory::MockType::MAKE_STRICT_MOCKS)),
        gcm_manager_("existing gcm registration id") {
    client_factory_->AddObserver(this);

    ExternalDeviceInfo unlock_key;
    unlock_key.set_public_key(kPublicKey1);
    unlock_key.set_friendly_device_name(kDeviceName1);
    unlock_key.set_bluetooth_address(kBluetoothAddress1);
    unlock_key.set_unlock_key(kUnlockKey1);
    unlock_key.set_unlockable(kUnlockable1);
    unlock_key.set_mobile_hotspot_supported(kMobileHotspotSupported1);
    BeaconSeed* seed1 = unlock_key.add_beacon_seeds();
    seed1->set_data(kBeaconSeed1Data);
    seed1->set_start_time_millis(kBeaconSeed1StartTime);
    seed1->set_end_time_millis(kBeaconSeed1EndTime);
    BeaconSeed* seed2 = unlock_key.add_beacon_seeds();
    seed2->set_data(kBeaconSeed2Data);
    seed2->set_start_time_millis(kBeaconSeed2StartTime);
    seed2->set_end_time_millis(kBeaconSeed2EndTime);
    unlock_key.set_arc_plus_plus(kArcPlusPlus1);
    unlock_key.set_pixel_phone(kPixelPhone1);
    devices_in_response_.push_back(unlock_key);

    ExternalDeviceInfo unlockable_device;
    unlockable_device.set_public_key(kPublicKey2);
    unlockable_device.set_friendly_device_name(kDeviceName2);
    unlockable_device.set_unlock_key(kUnlockKey2);
    unlockable_device.set_unlockable(kUnlockable2);
    unlockable_device.set_mobile_hotspot_supported(kMobileHotspotSupported2);
    BeaconSeed* seed3 = unlockable_device.add_beacon_seeds();
    seed3->set_data(kBeaconSeed3Data);
    seed3->set_start_time_millis(kBeaconSeed3StartTime);
    seed3->set_end_time_millis(kBeaconSeed3EndTime);
    BeaconSeed* seed4 = unlockable_device.add_beacon_seeds();
    seed4->set_data(kBeaconSeed4Data);
    seed4->set_start_time_millis(kBeaconSeed4StartTime);
    seed4->set_end_time_millis(kBeaconSeed4EndTime);
    unlockable_device.set_arc_plus_plus(kArcPlusPlus2);
    unlockable_device.set_pixel_phone(kPixelPhone2);
    devices_in_response_.push_back(unlockable_device);
  }

  ~CryptAuthDeviceManagerTest() {
    client_factory_->RemoveObserver(this);
  }

  // testing::Test:
  void SetUp() override {
    clock_.SetNow(base::Time::FromDoubleT(kInitialTimeNowSeconds));

    CryptAuthDeviceManager::RegisterPrefs(pref_service_.registry());
    pref_service_.SetUserPref(
        prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure,
        std::make_unique<base::Value>(false));
    pref_service_.SetUserPref(
        prefs::kCryptAuthDeviceSyncLastSyncTimeSeconds,
        std::make_unique<base::Value>(kLastSyncTimeSeconds));
    pref_service_.SetUserPref(
        prefs::kCryptAuthDeviceSyncReason,
        std::make_unique<base::Value>(INVOCATION_REASON_UNKNOWN));

    std::unique_ptr<base::DictionaryValue> device_dictionary(
        new base::DictionaryValue());

    std::string public_key_b64, device_name_b64, bluetooth_address_b64;
    base::Base64UrlEncode(kStoredPublicKey,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &public_key_b64);
    base::Base64UrlEncode(kStoredDeviceName,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &device_name_b64);
    base::Base64UrlEncode(kStoredBluetoothAddress,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &bluetooth_address_b64);

    device_dictionary->SetString("public_key", public_key_b64);
    device_dictionary->SetString("device_name", device_name_b64);
    device_dictionary->SetString("bluetooth_address",
                                     bluetooth_address_b64);
    device_dictionary->SetBoolean("unlock_key", kStoredUnlockKey);
    device_dictionary->SetBoolean("unlockable", kStoredUnlockable);
    device_dictionary->Set("beacon_seeds", std::make_unique<base::ListValue>());
    device_dictionary->SetBoolean("mobile_hotspot_supported",
                                  kStoredMobileHotspotSupported);
    {
      ListPrefUpdate update(&pref_service_,
                            prefs::kCryptAuthDeviceSyncUnlockKeys);
      update.Get()->Append(std::move(device_dictionary));
    }

    device_manager_.reset(new TestCryptAuthDeviceManager(
        &clock_, base::WrapUnique(client_factory_), &gcm_manager_,
        &pref_service_));
    device_manager_->AddObserver(this);

    get_my_devices_response_.add_devices()->CopyFrom(devices_in_response_[0]);
    get_my_devices_response_.add_devices()->CopyFrom(devices_in_response_[1]);

    ON_CALL(*sync_scheduler(), GetStrategy())
        .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  }

  void TearDown() override { device_manager_->RemoveObserver(this); }

  // CryptAuthDeviceManager::Observer:
  void OnSyncStarted() override { OnSyncStartedProxy(); }

  void OnSyncFinished(CryptAuthDeviceManager::SyncResult sync_result,
                      CryptAuthDeviceManager::DeviceChangeResult
                          device_change_result) override {
    OnSyncFinishedProxy(sync_result, device_change_result);
  }

  MOCK_METHOD0(OnSyncStartedProxy, void());
  MOCK_METHOD2(OnSyncFinishedProxy,
               void(CryptAuthDeviceManager::SyncResult,
                    CryptAuthDeviceManager::DeviceChangeResult));

  // Simulates firing the SyncScheduler to trigger a device sync attempt.
  void FireSchedulerForSync(
      InvocationReason expected_invocation_reason) {
    SyncScheduler::Delegate* delegate =
        static_cast<SyncScheduler::Delegate*>(device_manager_.get());

    std::unique_ptr<SyncScheduler::SyncRequest> sync_request =
        std::make_unique<SyncScheduler::SyncRequest>(
            device_manager_->GetSyncScheduler());
    EXPECT_CALL(*this, OnSyncStartedProxy());
    delegate->OnSyncRequested(std::move(sync_request));

    EXPECT_EQ(expected_invocation_reason,
              get_my_devices_request_.invocation_reason());

    // The allow_stale_read flag is set if the sync was not forced.
    bool allow_stale_read =
        pref_service_.GetInteger(prefs::kCryptAuthDeviceSyncReason) !=
        INVOCATION_REASON_UNKNOWN;
    EXPECT_EQ(allow_stale_read, get_my_devices_request_.allow_stale_read());
  }

  // MockCryptAuthClientFactory::Observer:
  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    EXPECT_CALL(*client, GetMyDevices(_, _, _, _))
        .WillOnce(DoAll(SaveArg<0>(&get_my_devices_request_),
                        SaveArg<1>(&success_callback_),
                        SaveArg<2>(&error_callback_)));
  }

  MockSyncScheduler* sync_scheduler() {
    return device_manager_->GetSyncScheduler().get();
  }

  base::SimpleTestClock clock_;

  // Owned by |device_manager_|.
  MockCryptAuthClientFactory* client_factory_;

  TestingPrefServiceSimple pref_service_;

  FakeCryptAuthGCMManager gcm_manager_;

  std::unique_ptr<TestCryptAuthDeviceManager> device_manager_;

  std::vector<ExternalDeviceInfo> devices_in_response_;

  GetMyDevicesResponse get_my_devices_response_;

  GetMyDevicesRequest get_my_devices_request_;

  CryptAuthClient::GetMyDevicesCallback success_callback_;

  CryptAuthClient::ErrorCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthDeviceManagerTest);
};

TEST_F(CryptAuthDeviceManagerTest, RegisterPrefs) {
  TestingPrefServiceSimple pref_service;
  CryptAuthDeviceManager::RegisterPrefs(pref_service.registry());
  EXPECT_TRUE(pref_service.FindPreference(
      prefs::kCryptAuthDeviceSyncLastSyncTimeSeconds));
  EXPECT_TRUE(pref_service.FindPreference(
      prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure));
  EXPECT_TRUE(pref_service.FindPreference(prefs::kCryptAuthDeviceSyncReason));
  EXPECT_TRUE(
      pref_service.FindPreference(prefs::kCryptAuthDeviceSyncUnlockKeys));
}

TEST_F(CryptAuthDeviceManagerTest, GetSyncState) {
  device_manager_->Start();

  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::PERIODIC_REFRESH));
  EXPECT_FALSE(device_manager_->IsRecoveringFromFailure());

  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  EXPECT_TRUE(device_manager_->IsRecoveringFromFailure());

  base::TimeDelta time_to_next_sync = base::TimeDelta::FromMinutes(60);
  ON_CALL(*sync_scheduler(), GetTimeToNextSync())
      .WillByDefault(Return(time_to_next_sync));
  EXPECT_EQ(time_to_next_sync, device_manager_->GetTimeToNextAttempt());

  ON_CALL(*sync_scheduler(), GetSyncState())
      .WillByDefault(Return(SyncScheduler::SyncState::SYNC_IN_PROGRESS));
  EXPECT_TRUE(device_manager_->IsSyncInProgress());

  ON_CALL(*sync_scheduler(), GetSyncState())
      .WillByDefault(Return(SyncScheduler::SyncState::WAITING_FOR_REFRESH));
  EXPECT_FALSE(device_manager_->IsSyncInProgress());
}

TEST_F(CryptAuthDeviceManagerTest, InitWithDefaultPrefs) {
  base::SimpleTestClock clock;
  clock.SetNow(base::Time::FromDoubleT(kInitialTimeNowSeconds));
  base::TimeDelta elapsed_time = clock.Now() - base::Time::FromDoubleT(0);

  TestingPrefServiceSimple pref_service;
  CryptAuthDeviceManager::RegisterPrefs(pref_service.registry());

  TestCryptAuthDeviceManager device_manager(
      &clock, std::make_unique<MockCryptAuthClientFactory>(
                  MockCryptAuthClientFactory::MockType::MAKE_STRICT_MOCKS),
      &gcm_manager_, &pref_service);

  EXPECT_CALL(
      *(device_manager.GetSyncScheduler()),
      Start(elapsed_time, SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  device_manager.Start();
  EXPECT_TRUE(device_manager.GetLastSyncTime().is_null());
  EXPECT_EQ(0u, device_manager.GetSyncedDevices().size());
}

TEST_F(CryptAuthDeviceManagerTest, InitWithExistingPrefs) {
  EXPECT_CALL(
      *sync_scheduler(),
      Start(clock_.Now() - base::Time::FromDoubleT(kLastSyncTimeSeconds),
            SyncScheduler::Strategy::PERIODIC_REFRESH));

  device_manager_->Start();
  EXPECT_EQ(base::Time::FromDoubleT(kLastSyncTimeSeconds),
            device_manager_->GetLastSyncTime());

  auto synced_devices = device_manager_->GetSyncedDevices();
  ASSERT_EQ(1u, synced_devices.size());
  EXPECT_EQ(kStoredPublicKey, synced_devices[0].public_key());
  EXPECT_EQ(kStoredDeviceName, synced_devices[0].friendly_device_name());
  EXPECT_EQ(kStoredBluetoothAddress, synced_devices[0].bluetooth_address());
  EXPECT_EQ(kStoredUnlockKey, synced_devices[0].unlock_key());
  EXPECT_EQ(kStoredUnlockable, synced_devices[0].unlockable());
}

TEST_F(CryptAuthDeviceManagerTest, SyncSucceedsForFirstTime) {
  pref_service_.ClearPref(prefs::kCryptAuthDeviceSyncLastSyncTimeSeconds);
  device_manager_->Start();

  FireSchedulerForSync(INVOCATION_REASON_INITIALIZATION);
  ASSERT_FALSE(success_callback_.is_null());

  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNowSeconds));
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));

  success_callback_.Run(get_my_devices_response_);
  EXPECT_EQ(clock_.Now(), device_manager_->GetLastSyncTime());

  ExpectSyncedDevicesAndPrefAreEqual(devices_in_response_,
                                     device_manager_->GetSyncedDevices(),
                                     pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, ForceSync) {
  device_manager_->Start();

  EXPECT_CALL(*sync_scheduler(), ForceSync());
  device_manager_->ForceSyncNow(INVOCATION_REASON_MANUAL);

  FireSchedulerForSync(INVOCATION_REASON_MANUAL);

  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNowSeconds));
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(get_my_devices_response_);
  EXPECT_EQ(clock_.Now(), device_manager_->GetLastSyncTime());

  ExpectSyncedDevicesAndPrefAreEqual(devices_in_response_,
                                     device_manager_->GetSyncedDevices(),
                                     pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, ForceSyncFailsThenSucceeds) {
  device_manager_->Start();
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure));
  base::Time old_sync_time = device_manager_->GetLastSyncTime();

  // The first force sync fails.
  EXPECT_CALL(*sync_scheduler(), ForceSync());
  device_manager_->ForceSyncNow(INVOCATION_REASON_MANUAL);
  FireSchedulerForSync(INVOCATION_REASON_MANUAL);
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNowSeconds));
  EXPECT_CALL(*this,
              OnSyncFinishedProxy(
                  CryptAuthDeviceManager::SyncResult::FAILURE,
                  CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED));
  error_callback_.Run("404");
  EXPECT_EQ(old_sync_time, device_manager_->GetLastSyncTime());
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure));
  EXPECT_EQ(static_cast<int>(INVOCATION_REASON_MANUAL),
            pref_service_.GetInteger(prefs::kCryptAuthDeviceSyncReason));

  // The second recovery sync succeeds.
  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  FireSchedulerForSync(INVOCATION_REASON_MANUAL);
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNowSeconds + 30));
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(get_my_devices_response_);
  EXPECT_EQ(clock_.Now(), device_manager_->GetLastSyncTime());

  ExpectSyncedDevicesAndPrefAreEqual(devices_in_response_,
                                     device_manager_->GetSyncedDevices(),
                                     pref_service_);

  EXPECT_FLOAT_EQ(
      clock_.Now().ToDoubleT(),
      pref_service_.GetDouble(prefs::kCryptAuthDeviceSyncLastSyncTimeSeconds));
  EXPECT_EQ(static_cast<int>(INVOCATION_REASON_UNKNOWN),
            pref_service_.GetInteger(prefs::kCryptAuthDeviceSyncReason));
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure));
}

TEST_F(CryptAuthDeviceManagerTest, PeriodicSyncFailsThenSucceeds) {
  device_manager_->Start();
  base::Time old_sync_time = device_manager_->GetLastSyncTime();

  // The first periodic sync fails.
  FireSchedulerForSync(INVOCATION_REASON_PERIODIC);
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNowSeconds));
  EXPECT_CALL(*this,
              OnSyncFinishedProxy(
                  CryptAuthDeviceManager::SyncResult::FAILURE,
                  CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED));
  error_callback_.Run("401");
  EXPECT_EQ(old_sync_time, device_manager_->GetLastSyncTime());
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure));

  // The second recovery sync succeeds.
  ON_CALL(*sync_scheduler(), GetStrategy())
      .WillByDefault(Return(SyncScheduler::Strategy::AGGRESSIVE_RECOVERY));
  FireSchedulerForSync(INVOCATION_REASON_FAILURE_RECOVERY);
  clock_.SetNow(base::Time::FromDoubleT(kLaterTimeNowSeconds + 30));
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(get_my_devices_response_);
  EXPECT_EQ(clock_.Now(), device_manager_->GetLastSyncTime());

  ExpectSyncedDevicesAndPrefAreEqual(devices_in_response_,
                                     device_manager_->GetSyncedDevices(),
                                     pref_service_);

  EXPECT_FLOAT_EQ(
      clock_.Now().ToDoubleT(),
      pref_service_.GetDouble(prefs::kCryptAuthDeviceSyncLastSyncTimeSeconds));
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kCryptAuthDeviceSyncIsRecoveringFromFailure));
}

TEST_F(CryptAuthDeviceManagerTest, SyncSameDevice) {
  device_manager_->Start();
  auto original_devices = device_manager_->GetSyncedDevices();

  // Sync new devices.
  FireSchedulerForSync(INVOCATION_REASON_PERIODIC);
  ASSERT_FALSE(success_callback_.is_null());
  EXPECT_CALL(*this,
              OnSyncFinishedProxy(
                  CryptAuthDeviceManager::SyncResult::SUCCESS,
                  CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED));

  // Sync the same device.
  ExternalDeviceInfo synced_device;
  synced_device.set_public_key(kStoredPublicKey);
  synced_device.set_friendly_device_name(kStoredDeviceName);
  synced_device.set_bluetooth_address(kStoredBluetoothAddress);
  synced_device.set_unlock_key(kStoredUnlockKey);
  synced_device.set_unlockable(kStoredUnlockable);
  synced_device.set_mobile_hotspot_supported(kStoredMobileHotspotSupported);
  GetMyDevicesResponse get_my_devices_response;
  get_my_devices_response.add_devices()->CopyFrom(synced_device);
  success_callback_.Run(get_my_devices_response);

  // Check that devices are still the same after sync.
  ExpectSyncedDevicesAndPrefAreEqual(
      original_devices, device_manager_->GetSyncedDevices(), pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, SyncEmptyDeviceList) {
  GetMyDevicesResponse empty_response;

  device_manager_->Start();
  EXPECT_EQ(1u, device_manager_->GetSyncedDevices().size());

  FireSchedulerForSync(INVOCATION_REASON_PERIODIC);
  ASSERT_FALSE(success_callback_.is_null());
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(empty_response);

  ExpectSyncedDevicesAndPrefAreEqual(
      std::vector<ExternalDeviceInfo>(),
      device_manager_->GetSyncedDevices(),
      pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, SyncThreeDevices) {
  GetMyDevicesResponse response(get_my_devices_response_);
  ExternalDeviceInfo synced_device2;
  synced_device2.set_public_key("new public key");
  synced_device2.set_friendly_device_name("new device name");
  synced_device2.set_bluetooth_address("aa:bb:cc:dd:ee:ff");
  synced_device2.set_unlock_key(true);
  response.add_devices()->CopyFrom(synced_device2);

  std::vector<ExternalDeviceInfo> expected_devices;
  expected_devices.push_back(devices_in_response_[0]);
  expected_devices.push_back(devices_in_response_[1]);
  expected_devices.push_back(synced_device2);

  device_manager_->Start();
  EXPECT_EQ(1u, device_manager_->GetSyncedDevices().size());
  EXPECT_EQ(1u, pref_service_.GetList(prefs::kCryptAuthDeviceSyncUnlockKeys)
                    ->GetSize());

  FireSchedulerForSync(INVOCATION_REASON_PERIODIC);
  ASSERT_FALSE(success_callback_.is_null());
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(response);

  ExpectSyncedDevicesAndPrefAreEqual(
      expected_devices, device_manager_->GetSyncedDevices(), pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, SyncOnGCMPushMessage) {
  device_manager_->Start();

  EXPECT_CALL(*sync_scheduler(), ForceSync());
  gcm_manager_.PushResyncMessage();

  FireSchedulerForSync(INVOCATION_REASON_SERVER_INITIATED);

  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(get_my_devices_response_);

  ExpectSyncedDevicesAndPrefAreEqual(devices_in_response_,
                                     device_manager_->GetSyncedDevices(),
                                     pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, SyncDeviceWithNoContents) {
  device_manager_->Start();

  EXPECT_CALL(*sync_scheduler(), ForceSync());
  gcm_manager_.PushResyncMessage();

  FireSchedulerForSync(INVOCATION_REASON_SERVER_INITIATED);

  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(get_my_devices_response_);

  ExpectSyncedDevicesAndPrefAreEqual(devices_in_response_,
                                     device_manager_->GetSyncedDevices(),
                                     pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, SyncFullyDetailedExternalDeviceInfos) {
  GetMyDevicesResponse response;

  // First, use a device with only a public key (a public key is the only
  // required field). This ensures devices work properly when they do not have
  // all fields filled out.
  ExternalDeviceInfo device_with_only_public_key;
  device_with_only_public_key.set_public_key("publicKey1");
  // Currently, CryptAuthDeviceManager only stores devices which are unlock
  // keys, so set_unlock_key(true) must be called here for storage to work.
  // TODO(khorimoto): Remove this when support for storing all types of devices
  // is added.
  device_with_only_public_key.set_unlock_key(true);
  response.add_devices()->CopyFrom(device_with_only_public_key);

  // Second, use a device with all fields filled out. This ensures that all
  // device details are properly saved.
  ExternalDeviceInfo device_with_all_fields;
  device_with_all_fields.set_public_key("publicKey2");
  device_with_all_fields.set_friendly_device_name("deviceName");
  device_with_all_fields.set_bluetooth_address("aa:bb:cc:dd:ee:ff");
  device_with_all_fields.set_unlock_key(true);
  device_with_all_fields.set_unlockable(true);
  device_with_all_fields.set_last_update_time_millis(123456789L);
  device_with_all_fields.set_mobile_hotspot_supported(true);
  device_with_all_fields.set_device_type(DeviceType::ANDROIDOS);
  BeaconSeed seed1;
  seed1.set_data(kBeaconSeed1Data);
  seed1.set_start_time_millis(kBeaconSeed1StartTime);
  seed1.set_end_time_millis(kBeaconSeed1EndTime);
  device_with_all_fields.add_beacon_seeds()->CopyFrom(seed1);
  BeaconSeed seed2;
  seed2.set_data(kBeaconSeed2Data);
  seed2.set_start_time_millis(kBeaconSeed2StartTime);
  seed2.set_end_time_millis(kBeaconSeed2EndTime);
  device_with_all_fields.add_beacon_seeds()->CopyFrom(seed2);
  response.add_devices()->CopyFrom(device_with_all_fields);

  std::vector<ExternalDeviceInfo> expected_devices;
  expected_devices.push_back(device_with_only_public_key);
  expected_devices.push_back(device_with_all_fields);

  device_manager_->Start();
  FireSchedulerForSync(INVOCATION_REASON_PERIODIC);
  ASSERT_FALSE(success_callback_.is_null());
  EXPECT_CALL(*this, OnSyncFinishedProxy(
                         CryptAuthDeviceManager::SyncResult::SUCCESS,
                         CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(response);

  ExpectSyncedDevicesAndPrefAreEqual(
      expected_devices, device_manager_->GetSyncedDevices(), pref_service_);
}

TEST_F(CryptAuthDeviceManagerTest, SubsetsOfSyncedDevices) {
  device_manager_->Start();

  FireSchedulerForSync(INVOCATION_REASON_PERIODIC);
  ASSERT_FALSE(success_callback_.is_null());
  EXPECT_CALL(*this,
              OnSyncFinishedProxy(
                  CryptAuthDeviceManager::SyncResult::SUCCESS,
                  CryptAuthDeviceManager::DeviceChangeResult::CHANGED));
  success_callback_.Run(get_my_devices_response_);

  // All synced devices.
  ExpectSyncedDevicesAndPrefAreEqual(
      devices_in_response_,
      device_manager_->GetSyncedDevices(),
      pref_service_);

  // Only unlock keys.
  ExpectSyncedDevicesAreEqual(
      std::vector<ExternalDeviceInfo>(1, devices_in_response_[0]),
      device_manager_->GetUnlockKeys());

  // Only tether hosts.
  ExpectSyncedDevicesAreEqual(
      std::vector<ExternalDeviceInfo>(1, devices_in_response_[0]),
      device_manager_->GetTetherHosts());
}

}  // namespace cryptauth
