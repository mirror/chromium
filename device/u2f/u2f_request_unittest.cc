// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_io_thread.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "device/test/usb_test_gadget.h"
#include "device/u2f/fake_hid_impl_for_testing.h"
#include "device/u2f/u2f_hid_device.h"
#include "device/u2f/u2f_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace {

class FakeU2fRequest : public U2fRequest {
 public:
  FakeU2fRequest(const ResponseCallback& cb,
                 device::mojom::HidManager* hid_manager)
      : U2fRequest(cb, hid_manager) {}
  ~FakeU2fRequest() override {}

  void TryDevice() override {
    cb_.Run(U2fReturnCode::SUCCESS, std::vector<uint8_t>());
  }
};
}  // namespace

class TestResponseCallback {
 public:
  TestResponseCallback()
      : callback_(base::Bind(&TestResponseCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestResponseCallback() {}

  void ReceivedCallback(U2fReturnCode status,
                        const std::vector<uint8_t>& data) {
    closure_.Run();
  }

  void WaitForCallback() {
    base::RunLoop run_loop;
    closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  U2fRequest::ResponseCallback& callback() { return callback_; }

 private:
  base::Closure closure_;
  U2fRequest::ResponseCallback callback_;
};

class U2fRequestTest : public testing::Test {
 public:
  U2fRequestTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        io_thread_(base::TestIOThread::kAutoStart) {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::TestIOThread io_thread_;
};

TEST_F(U2fRequestTest, TestAddRemoveDevice) {
  device::mojom::HidManagerPtr hid_manager;
  mojo::MakeRequest(&hid_manager);

  TestResponseCallback cb;
  FakeU2fRequest request(cb.callback(), hid_manager.get());
  request.Enumerate();
  EXPECT_EQ(static_cast<size_t>(0), request.devices_.size());

  HidCollectionInfo c_info;
  c_info.usage = HidUsageAndPage(1, 0xf1d0);

  // Add one U2F device
  auto u2f_device = std::make_unique<device::mojom::HidDeviceInfo>();
  u2f_device->guid = "A";
  u2f_device->product_name = "Test Fido Device";
  u2f_device->serial_number = "123FIDO";
  u2f_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  u2f_device->collections.push_back(c_info);
  u2f_device->max_input_report_size = 64;
  u2f_device->max_output_report_size = 64;

  request.DeviceAdded(u2f_device->Clone());
  EXPECT_EQ(static_cast<size_t>(1), request.devices_.size());

  // Add one non-U2F device. Verify that it is not added to our device list.
  auto other_device = std::make_unique<device::mojom::HidDeviceInfo>();
  other_device->guid = "B";
  other_device->product_name = "Other Device";
  other_device->serial_number = "OtherDevice";
  other_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;

  request.DeviceAdded(other_device->Clone());
  EXPECT_EQ(static_cast<size_t>(1), request.devices_.size());

  // Remove the non-U2F device and verify that device list was unchanged.
  request.DeviceRemoved(other_device->Clone());
  EXPECT_EQ(static_cast<size_t>(1), request.devices_.size());

  // Remove the U2F device and verify that device list is empty.
  request.DeviceRemoved(u2f_device->Clone());
  EXPECT_EQ(static_cast<size_t>(0), request.devices_.size());
}

TEST_F(U2fRequestTest, TestIterateDevice) {
  device::mojom::HidManagerPtr hid_manager;
  FakeHidManager fake_hid_manager_impl(mojo::MakeRequest(&hid_manager));

  TestResponseCallback cb;
  FakeU2fRequest request(cb.callback(), hid_manager.get());
  HidCollectionInfo c_info;
  c_info.usage = HidUsageAndPage(1, 0xf1d0);

  // Add one U2F device and one non-U2f device
  device::mojom::HidDeviceInfoPtr u2f_device =
      device::mojom::HidDeviceInfo::New();
  u2f_device->guid = "A";
  u2f_device->product_name = "Test Fido device";
  u2f_device->serial_number = "123FIDO";
  u2f_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  u2f_device->collections.push_back(c_info);
  u2f_device->max_input_report_size = 64;
  u2f_device->max_output_report_size = 64;

  request.devices_.push_back(
      std::make_unique<U2fHidDevice>(std::move(u2f_device), hid_manager.get()));

  device::mojom::HidDeviceInfoPtr other_device =
      device::mojom::HidDeviceInfo::New();
  other_device->guid = "B";
  other_device->product_name = "Other Device";
  other_device->serial_number = "OtherDevice";
  other_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;

  request.devices_.push_back(std::make_unique<U2fHidDevice>(
      std::move(other_device), hid_manager.get()));

  // Move first device to current
  request.IterateDevice();
  ASSERT_NE(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(1), request.devices_.size());

  // Move second device to current, first to attempted
  request.IterateDevice();
  ASSERT_NE(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(1), request.attempted_devices_.size());

  // Move second device from current to attempted, move attempted to devices as
  // all devices have been attempted
  request.IterateDevice();
  ASSERT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(2), request.devices_.size());
  EXPECT_EQ(static_cast<size_t>(0), request.attempted_devices_.size());
}

TEST_F(U2fRequestTest, TestBasicMachine) {
  device::mojom::HidManagerPtr hid_manager;
  FakeHidManager fake_hid_manager_impl(mojo::MakeRequest(&hid_manager));

  TestResponseCallback cb;
  FakeU2fRequest request(cb.callback(), hid_manager.get());
  request.Start();

  HidCollectionInfo c_info;
  c_info.usage = HidUsageAndPage(1, 0xf1d0);

  // Add one U2F device
  device::mojom::HidDeviceInfoPtr u2f_device =
      device::mojom::HidDeviceInfo::New();
  u2f_device->guid = "A";
  u2f_device->product_name = "Test Fido device";
  u2f_device->serial_number = "123FIDO";
  u2f_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  u2f_device->collections.push_back(c_info);
  u2f_device->max_input_report_size = 64;
  u2f_device->max_output_report_size = 64;

  fake_hid_manager_impl.AddDevice(std::move(u2f_device));

  cb.WaitForCallback();
  EXPECT_EQ(U2fRequest::State::BUSY, request.state_);
}

}  // namespace device
