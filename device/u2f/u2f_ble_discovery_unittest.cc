// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_discovery.h"

#include "base/bind.h"
#include "build/build_config.h"
#include "device/bluetooth/test/bluetooth_test.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "device/bluetooth/test/bluetooth_test_android.h"
#elif defined(OS_MACOSX)
#include "device/bluetooth/test/bluetooth_test_mac.h"
#elif defined(OS_WIN)
#include "device/bluetooth/test/bluetooth_test_win.h"
#elif defined(OS_CHROMEOS) || defined(OS_LINUX)
#include "device/bluetooth/test/bluetooth_test_bluez.h"
#endif

namespace device {

class DiscoveryStartedCallbackReceiver {
 public:
  DiscoveryStartedCallbackReceiver() = default;
  ~DiscoveryStartedCallbackReceiver() = default;

  U2fBleDiscovery::DiscoveryStartedCallback GetCallback() {
    return base::Bind(&DiscoveryStartedCallbackReceiver::Callback,
                      base::Unretained(this));
  }

  void WaitForStarted() {
    if (!callback_count_) {
      base::RunLoop run_loop;
      quit_closure_ = run_loop.QuitClosure();
      run_loop.Run();
    }
  }

  void ExpectReceivedOnce(bool started) const {
    EXPECT_EQ(1, callback_count_);
    EXPECT_EQ(started, started_);
  }

 private:
  void Callback(bool started) {
    started_ = started;
    ++callback_count_;
    if (!quit_closure_.is_null())
      quit_closure_.Run();
  }

  bool started_ = false;
  int callback_count_ = 0;
  base::Closure quit_closure_;

  DISALLOW_COPY_AND_ASSIGN(DiscoveryStartedCallbackReceiver);
};

class DiscoveryCallbackReceiver {
 public:
  DiscoveryCallbackReceiver() = default;
  ~DiscoveryCallbackReceiver() = default;

  U2fBleDiscovery::DeviceStatusCallback GetCallback() {
    return base::Bind(&DiscoveryCallbackReceiver::Callback,
                      base::Unretained(this));
  }

  void WaitForDiscovery() {
    ASSERT_LE(wait_count_, callback_count_);
    ASSERT_LE(callback_count_, wait_count_ + 1);

    if (wait_count_++ == callback_count_) {
      base::RunLoop run_loop;
      quit_closure_ = run_loop.QuitClosure();
      run_loop.Run();
    }
  }

  void ExpectReceivedOnce(const std::string& address,
                          U2fBleDiscovery::DeviceStatus status) const {
    EXPECT_EQ(wait_count_, callback_count_);
    EXPECT_EQ(address_, address);
    EXPECT_EQ(status_, status);
  }

 private:
  void Callback(std::string address, U2fBleDiscovery::DeviceStatus status) {
    address_ = std::move(address);
    status_ = status;
    if (wait_count_ > callback_count_)
      quit_closure_.Run();
    ++callback_count_;
  }

  std::string address_;
  U2fBleDiscovery::DeviceStatus status_;
  int wait_count_ = 0;
  int callback_count_ = 0;
  base::Closure quit_closure_;

  DISALLOW_COPY_AND_ASSIGN(DiscoveryCallbackReceiver);
};

TEST_F(BluetoothTest, DiscoverKnownDeviceAndRemove) {
  if (!PlatformSupportsLowEnergy()) {
    LOG(WARNING) << "Low Energy Bluetooth unavailable, skipping unit test.";
    return;
  }
  InitWithFakeAdapter();

  SimulateLowEnergyDevice(4);  // This device should be ignored.
  SimulateLowEnergyDevice(7);

  DiscoveryCallbackReceiver receiver;
  DiscoveryStartedCallbackReceiver started_receiver;

  U2fBleDiscovery discovery;
  discovery.Start(started_receiver.GetCallback(), receiver.GetCallback());
  started_receiver.WaitForStarted();
  started_receiver.ExpectReceivedOnce(true);

  receiver.WaitForDiscovery();
  receiver.ExpectReceivedOnce(BluetoothTestBase::kTestDeviceAddress1,
                              U2fBleDiscovery::DeviceStatus::KNOWN);

  /*
  DeleteDevice(device);
  receiver.WaitForDiscovery();
  receiver.ExpectReceivedOnce(BluetoothTestBase::kTestDeviceAddress1,
                              U2fBleDiscovery::DeviceStatus::REMOVED);
  */

  discovery.Stop();
}

TEST_F(BluetoothTest, DiscoverNewDeviceAndRemove) {
  if (!PlatformSupportsLowEnergy()) {
    LOG(WARNING) << "Low Energy Bluetooth unavailable, skipping unit test.";
    return;
  }
  InitWithFakeAdapter();

  DiscoveryCallbackReceiver receiver;
  DiscoveryStartedCallbackReceiver started_receiver;

  U2fBleDiscovery discovery;
  discovery.Start(started_receiver.GetCallback(), receiver.GetCallback());
  started_receiver.WaitForStarted();
  started_receiver.ExpectReceivedOnce(true);

  SimulateLowEnergyDevice(4);  // This device should be ignored.
  SimulateLowEnergyDevice(7);

  receiver.WaitForDiscovery();
  receiver.ExpectReceivedOnce(BluetoothTestBase::kTestDeviceAddress1,
                              U2fBleDiscovery::DeviceStatus::ADDED);

  /*
  DeleteDevice(device);
  receiver.WaitForDiscovery();
  receiver.ExpectReceivedOnce(BluetoothTestBase::kTestDeviceAddress1,
                              U2fBleDiscovery::DeviceStatus::REMOVED);
  */

  discovery.Stop();
}

}  // namespace device
