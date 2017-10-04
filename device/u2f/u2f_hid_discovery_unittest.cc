// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_hid_discovery.h"

#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "device/base/mock_device_client.h"
#include "device/hid/hid_collection_info.h"
#include "device/hid/mock_hid_service.h"
#include "device/u2f/u2f_hid_device.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

#if defined(OS_MACOSX)
HidPlatformDeviceId kTestDeviceId0 = 42;
HidPlatformDeviceId kTestDeviceId1 = 43;
HidPlatformDeviceId kTestDeviceId2 = 44;
#else
HidPlatformDeviceId kTestDeviceId0 = "device0";
HidPlatformDeviceId kTestDeviceId1 = "device1";
HidPlatformDeviceId kTestDeviceId2 = "device2";
#endif

scoped_refptr<HidDeviceInfo> MakeU2fDevice0() {
  HidCollectionInfo c_info;
  c_info.usage = HidUsageAndPage(1, static_cast<HidUsageAndPage::Page>(0xf1d0));
  return new HidDeviceInfo(kTestDeviceId0, 0, 0, "Test Fido Device", "123FIDO",
                           device::mojom::HidBusType::kHIDBusTypeUSB, c_info,
                           64, 64, 0);
}

scoped_refptr<HidDeviceInfo> MakeU2fDevice1() {
  HidCollectionInfo c_info;
  c_info.usage = HidUsageAndPage(1, static_cast<HidUsageAndPage::Page>(0xf1d0));
  return new HidDeviceInfo(kTestDeviceId1, 0, 0, "Test Fido Device", "123FIDO",
                           device::mojom::HidBusType::kHIDBusTypeUSB, c_info,
                           64, 64, 0);
}

scoped_refptr<HidDeviceInfo> MakeOtherDevice() {
  return new HidDeviceInfo(kTestDeviceId2, 0, 0, "Other Device", "OtherDevice",
                           device::mojom::HidBusType::kHIDBusTypeUSB,
                           std::vector<uint8_t>());
}

}  // namespace

class MockDiscoveryReceiver {
 public:
  MOCK_METHOD2(OnDeviceDiscoveredRaw,
               void(const std::string&, U2fDiscovery::DeviceStatus));

  // This wrapper is added to cope with GMock's inability to work with move-only
  // types as well as to avoid introducing a custom matcher to compare devices.
  void OnDeviceDiscovered(std::unique_ptr<U2fDevice> device,
                          U2fDiscovery::DeviceStatus status) {
    OnDeviceDiscoveredRaw(device->GetId(), status);
  }
};

class U2fHidDiscoveryTest : public testing::Test {
 public:
  base::test::ScopedTaskEnvironment& scoped_task_environment() {
    return scoped_task_environment_;
  }

  MockHidService* hid_service() { return device_client_.hid_service(); }

  void SetUp() override { hid_service()->FirstEnumerationComplete(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  device::MockDeviceClient device_client_;
};

TEST_F(U2fHidDiscoveryTest, TestAddRemoveDevice) {
  U2fHidDiscovery discovery;
  MockDiscoveryReceiver receiver;

  auto u2f_device_0 = MakeU2fDevice0();
  std::string device_id_0 =
      U2fHidDevice(u2f_device_0->device()->Clone()).GetId();
  hid_service()->AddDevice(u2f_device_0);

  auto do_nothing = [](bool) {};
  discovery.Start(
      base::Bind(do_nothing),
      base::BindRepeating(&MockDiscoveryReceiver::OnDeviceDiscovered,
                          base::Unretained(&receiver)));

  // Devices initially known to the service before discovery started should be
  // reported as KNOWN.
  EXPECT_CALL(receiver, OnDeviceDiscoveredRaw(
                            device_id_0, U2fDiscovery::DeviceStatus::KNOWN));
  scoped_task_environment().RunUntilIdle();

  // Devices added during the discovery should be reported as ADDED.
  auto u2f_device_1 = MakeU2fDevice1();
  std::string device_id_1 =
      U2fHidDevice(u2f_device_1->device()->Clone()).GetId();
  EXPECT_CALL(receiver, OnDeviceDiscoveredRaw(
                            device_id_1, U2fDiscovery::DeviceStatus::ADDED));
  hid_service()->AddDevice(u2f_device_1);

  // Added non-U2F devices should not be reported at all.
  auto other_device = MakeOtherDevice();
  EXPECT_CALL(receiver, OnDeviceDiscoveredRaw(testing::_, testing::_)).Times(0);
  hid_service()->AddDevice(other_device);

  // Removed non-U2F devices should not be reported at all.
  EXPECT_CALL(receiver, OnDeviceDiscoveredRaw(testing::_, testing::_)).Times(0);
  hid_service()->RemoveDevice(other_device->platform_device_id());

  // Removed U2F devices should be reported as REMOVED.
  EXPECT_CALL(receiver, OnDeviceDiscoveredRaw(
                            device_id_0, U2fDiscovery::DeviceStatus::REMOVED));
  EXPECT_CALL(receiver, OnDeviceDiscoveredRaw(
                            device_id_1, U2fDiscovery::DeviceStatus::REMOVED));
  hid_service()->RemoveDevice(u2f_device_0->platform_device_id());
  hid_service()->RemoveDevice(u2f_device_1->platform_device_id());
}

}  // namespace device
