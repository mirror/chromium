// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "device/u2f/fake_hid_impl_for_testing.h"
#include "device/u2f/u2f_apdu_command.h"
#include "device/u2f/u2f_apdu_response.h"
#include "device/u2f/u2f_hid_device.h"
#include "device/u2f/u2f_message.h"
#include "device/u2f/u2f_packet.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/cpp/hid/hid_device_filter.h"
#include "services/device/public/interfaces/hid.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace {

void ResponseCallback(std::unique_ptr<device::U2fApduResponse>* output,
                      bool success,
                      std::unique_ptr<device::U2fApduResponse> response) {
  *output = std::move(response);
}

}  // namespace

namespace device {

class U2fDeviceEnumerate {
 public:
  explicit U2fDeviceEnumerate(device::mojom::HidManager* hid_manager)
      : closure_(),
        callback_(base::BindOnce(&U2fDeviceEnumerate::ReceivedCallback,
                                 base::Unretained(this))),
        hid_manager_(hid_manager),
        run_loop_() {}
  ~U2fDeviceEnumerate() {}

  void ReceivedCallback(std::vector<device::mojom::HidDeviceInfoPtr> devices) {
    std::list<std::unique_ptr<U2fHidDevice>> u2f_devices;
    filter_.SetUsagePage(0xf1d0);
    for (auto& device_info : devices) {
      if (filter_.Matches(*device_info))
        u2f_devices.push_front(std::make_unique<U2fHidDevice>(
            std::move(device_info), hid_manager_));
    }
    devices_ = std::move(u2f_devices);
    closure_.Run();
  }

  std::list<std::unique_ptr<U2fHidDevice>>& WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return devices_;
  }

  device::mojom::HidManager::GetDevicesCallback callback() {
    return std::move(callback_);
  }

 private:
  HidDeviceFilter filter_;
  std::list<std::unique_ptr<U2fHidDevice>> devices_;
  base::Closure closure_;
  device::mojom::HidManager::GetDevicesCallback callback_;
  device::mojom::HidManager* hid_manager_;
  base::RunLoop run_loop_;
};

class TestVersionCallback {
 public:
  TestVersionCallback()
      : closure_(),
        callback_(base::Bind(&TestVersionCallback::ReceivedCallback,
                             base::Unretained(this))),
        run_loop_() {}
  ~TestVersionCallback() {}

  void ReceivedCallback(bool success, U2fDevice::ProtocolVersion version) {
    version_ = version;
    closure_.Run();
  }

  U2fDevice::ProtocolVersion WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return version_;
  }

  const U2fDevice::VersionCallback& callback() { return callback_; }

 private:
  U2fDevice::ProtocolVersion version_;
  base::Closure closure_;
  U2fDevice::VersionCallback callback_;
  base::RunLoop run_loop_;
};

class TestDeviceCallback {
 public:
  TestDeviceCallback()
      : closure_(),
        callback_(base::Bind(&TestDeviceCallback::ReceivedCallback,
                             base::Unretained(this))),
        run_loop_() {}
  ~TestDeviceCallback() {}

  void ReceivedCallback(bool success,
                        std::unique_ptr<U2fApduResponse> response) {
    response_ = std::move(response);
    closure_.Run();
  }

  std::unique_ptr<U2fApduResponse> WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return std::move(response_);
  }

  const U2fDevice::DeviceCallback& callback() { return callback_; }

 private:
  std::unique_ptr<U2fApduResponse> response_;
  base::Closure closure_;
  U2fDevice::DeviceCallback callback_;
  base::RunLoop run_loop_;
};

class U2fHidDeviceTest : public testing::Test {
 public:
  U2fHidDeviceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void SetUp() override {
    fake_hid_manager_ = std::make_unique<FakeHidManager>();
    fake_hid_manager_->AddBinding2(mojo::MakeRequest(&hid_manager_));
  }

 protected:
  device::mojom::HidManagerPtr hid_manager_;
  std::unique_ptr<FakeHidManager> fake_hid_manager_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(U2fHidDeviceTest, TestHidDeviceVersion) {
  if (!U2fHidDevice::IsTestEnabled())
    return;

  U2fDeviceEnumerate callback(hid_manager_.get());
  hid_manager_->GetDevices(callback.callback());
  std::list<std::unique_ptr<U2fHidDevice>>& u2f_devices =
      callback.WaitForCallback();

  for (auto& device : u2f_devices) {
    TestVersionCallback vc;
    device->Version(vc.callback());
    U2fDevice::ProtocolVersion version = vc.WaitForCallback();
    EXPECT_EQ(version, U2fDevice::ProtocolVersion::U2F_V2);
  }
};

TEST_F(U2fHidDeviceTest, TestMultipleRequests) {
  if (!U2fHidDevice::IsTestEnabled())
    return;

  U2fDeviceEnumerate callback(hid_manager_.get());
  hid_manager_->GetDevices(callback.callback());
  std::list<std::unique_ptr<U2fHidDevice>>& u2f_devices =
      callback.WaitForCallback();

  for (auto& device : u2f_devices) {
    TestVersionCallback vc;
    TestVersionCallback vc2;
    // Call version twice to check message queueing
    device->Version(vc.callback());
    device->Version(vc2.callback());
    U2fDevice::ProtocolVersion version = vc.WaitForCallback();
    EXPECT_EQ(version, U2fDevice::ProtocolVersion::U2F_V2);
    version = vc2.WaitForCallback();
    EXPECT_EQ(version, U2fDevice::ProtocolVersion::U2F_V2);
  }
};

TEST_F(U2fHidDeviceTest, TestConnectionFailure) {
  // Setup and enumerate mock device
  U2fDeviceEnumerate callback(hid_manager_.get());

  auto c_info = device::mojom::HidCollectionInfo::New();
  c_info->usage = device::mojom::HidUsageAndPage::New(1, 0xf1d0);

  auto hid_device = device::mojom::HidDeviceInfo::New();
  hid_device->guid = "A";
  hid_device->product_name = "Test Fido device";
  hid_device->serial_number = "123FIDO";
  hid_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  hid_device->collections.push_back(std::move(c_info));
  hid_device->max_input_report_size = 64;
  hid_device->max_output_report_size = 64;

  fake_hid_manager_->AddDevice(std::move(hid_device));
  hid_manager_->GetDevices(callback.callback());

  std::list<std::unique_ptr<U2fHidDevice>>& u2f_devices =
      callback.WaitForCallback();

  ASSERT_EQ(static_cast<size_t>(1), u2f_devices.size());
  auto& device = u2f_devices.front();
  // Put device in IDLE state
  TestDeviceCallback cb0;
  device->state_ = U2fHidDevice::State::IDLE;

  // Manually delete connection
  device->connection_ = nullptr;
  // Add pending transactions manually and ensure they are processed
  std::unique_ptr<U2fApduResponse> response1(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->pending_transactions_.push_back(
      {U2fApduCommand::CreateVersion(),
       base::Bind(&ResponseCallback, &response1)});
  std::unique_ptr<U2fApduResponse> response2(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->pending_transactions_.push_back(
      {U2fApduCommand::CreateVersion(),
       base::Bind(&ResponseCallback, &response2)});
  std::unique_ptr<U2fApduResponse> response3(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->DeviceTransact(U2fApduCommand::CreateVersion(),
                         base::Bind(&ResponseCallback, &response3));
  EXPECT_EQ(U2fHidDevice::State::DEVICE_ERROR, device->state_);
  EXPECT_EQ(nullptr, response1);
  EXPECT_EQ(nullptr, response2);
  EXPECT_EQ(nullptr, response3);
};

TEST_F(U2fHidDeviceTest, TestDeviceError) {
  // Setup and enumerate mock device
  U2fDeviceEnumerate callback(hid_manager_.get());

  auto c_info = device::mojom::HidCollectionInfo::New();
  c_info->usage = device::mojom::HidUsageAndPage::New(1, 0xf1d0);

  auto hid_device = device::mojom::HidDeviceInfo::New();
  hid_device->guid = "A";
  hid_device->product_name = "Test Fido device";
  hid_device->serial_number = "123FIDO";
  hid_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  hid_device->collections.push_back(std::move(c_info));
  hid_device->max_input_report_size = 64;
  hid_device->max_output_report_size = 64;

  fake_hid_manager_->AddDevice(std::move(hid_device));
  hid_manager_->GetDevices(callback.callback());

  std::list<std::unique_ptr<U2fHidDevice>>& u2f_devices =
      callback.WaitForCallback();

  ASSERT_EQ(static_cast<size_t>(1), u2f_devices.size());
  auto& device = u2f_devices.front();
  // Mock connection where writes always fail
  FakeHidConnection::mock_connection_error_ = true;
  device->state_ = U2fHidDevice::State::IDLE;
  std::unique_ptr<U2fApduResponse> response0(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->DeviceTransact(U2fApduCommand::CreateVersion(),
                         base::Bind(&ResponseCallback, &response0));
  EXPECT_EQ(nullptr, response0);
  EXPECT_EQ(U2fHidDevice::State::DEVICE_ERROR, device->state_);

  // Add pending transactions manually and ensure they are processed
  std::unique_ptr<U2fApduResponse> response1(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->pending_transactions_.push_back(
      {U2fApduCommand::CreateVersion(),
       base::Bind(&ResponseCallback, &response1)});
  std::unique_ptr<U2fApduResponse> response2(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->pending_transactions_.push_back(
      {U2fApduCommand::CreateVersion(),
       base::Bind(&ResponseCallback, &response2)});
  std::unique_ptr<U2fApduResponse> response3(
      U2fApduResponse::CreateFromMessage(std::vector<uint8_t>({0x0, 0x0})));
  device->DeviceTransact(U2fApduCommand::CreateVersion(),
                         base::Bind(&ResponseCallback, &response3));
  FakeHidConnection::mock_connection_error_ = false;

  EXPECT_EQ(U2fHidDevice::State::DEVICE_ERROR, device->state_);
  EXPECT_EQ(nullptr, response1);
  EXPECT_EQ(nullptr, response2);
  EXPECT_EQ(nullptr, response3);
};

TEST_F(U2fHidDeviceTest, TestRetryChannelAllocation) {
  // Setup and enumerate mock device
  U2fDeviceEnumerate callback(hid_manager_.get());

  auto c_info = device::mojom::HidCollectionInfo::New();
  c_info->usage = device::mojom::HidUsageAndPage::New(1, 0xf1d0);

  auto hid_device = device::mojom::HidDeviceInfo::New();
  hid_device->guid = "A";
  hid_device->product_name = "Test Fido device";
  hid_device->serial_number = "123FIDO";
  hid_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  hid_device->collections.push_back(std::move(c_info));
  hid_device->max_input_report_size = 64;
  hid_device->max_output_report_size = 64;

  fake_hid_manager_->AddDevice(std::move(hid_device));
  hid_manager_->GetDevices(callback.callback());

  std::list<std::unique_ptr<U2fHidDevice>>& u2f_devices =
      callback.WaitForCallback();

  ASSERT_EQ(static_cast<size_t>(1), u2f_devices.size());
  auto& device = u2f_devices.front();

  // Replace device HID connection with custom client connection bound to mock
  // server-side mojom connection.
  device::mojom::HidConnectionPtr connection_client;
  MockHidConnection mock_connection(device->device_info_.Clone(),
                                    mojo::MakeRequest(&connection_client));
  device->connection_ = std::move(connection_client);

  static const std::vector<uint8_t> nonce_set_by_client = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  static const std::vector<uint8_t> buffer_with_incorrect_nonce = {
      // clang-format off
      0x00, 0x00, 0x00, 0x00, 0x00, // packet header
      0x00, 0x11, // payload size (17)
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Incorrect nonce
      0x01, 0x02, 0x03, 0x04,        // Dummy channel id
      0x00, 0x00, 0x00, 0x00, 0x00,  // Device version data
      // zero padding for HID packet below
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // clang-format on
  };

  // Once OnAllocateChannel is invoked, Read() must be invoked since mock hid
  // connection will response with a different nonce value.
  EXPECT_CALL(mock_connection, ReadPtr());
  TestDeviceCallback cb0;
  device->OnAllocateChannel(
      nonce_set_by_client, nullptr, std::move(cb0.callback()), true,
      U2fMessage::CreateFromSerializedData(buffer_with_incorrect_nonce));

  // To prevent proceeding to next step in device level transition, set state to
  // error.
  device->state_ = U2fHidDevice::State::DEVICE_ERROR;
  // Dispatch mojom request
  base::RunLoop().RunUntilIdle();
  // Wait for device request to finish (will take 3000ms).
  cb0.WaitForCallback();
};

}  // namespace device
