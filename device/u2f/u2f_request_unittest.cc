// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_io_thread.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "device/test/test_device_client.h"
#include "device/test/usb_test_gadget.h"
#include "device/u2f/mock_u2f_device.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "device/u2f/u2f_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace {

class FakeU2fRequest : public U2fRequest {
 public:
  FakeU2fRequest(std::vector<std::unique_ptr<U2fDiscovery>> discoveries,
                 const ResponseCallback& cb)
      : U2fRequest(std::move(discoveries), cb) {}
  ~FakeU2fRequest() override {}

  void TryDevice() override {
    cb_.Run(U2fReturnCode::SUCCESS, std::vector<uint8_t>());
  }
};

}  // namespace

class U2fRequestTest : public testing::Test {
 public:
  base::test::ScopedTaskEnvironment& scoped_task_environment() {
    return scoped_task_environment_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(U2fRequestTest, TestIterateDevice) {
  // No discoveries are needed, since |request.devices_| is accessed directly.
  auto do_nothing = [](U2fReturnCode, const std::vector<uint8_t>&) {};
  FakeU2fRequest request({}, base::Bind(do_nothing));

  // Add two U2F devices
  request.devices_.push_back(std::make_unique<MockU2fDevice>());
  request.devices_.push_back(std::make_unique<MockU2fDevice>());

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
  scoped_task_environment().RunUntilIdle();

  ASSERT_EQ(nullptr, request.current_device_);
  EXPECT_EQ(static_cast<size_t>(2), request.devices_.size());
  EXPECT_EQ(static_cast<size_t>(0), request.attempted_devices_.size());
}

TEST_F(U2fRequestTest, TestBasicMachine) {
  auto discovery = std::make_unique<MockU2fDiscovery>();
  MockU2fDiscovery* discovery_raw = discovery.get();

  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  discoveries.push_back(std::move(discovery));

  auto do_nothing = [](U2fReturnCode, const std::vector<uint8_t>&) {};
  FakeU2fRequest request(std::move(discoveries), base::Bind(do_nothing));

  EXPECT_CALL(*discovery_raw, StartImpl(testing::_))
      .WillOnce(testing::Invoke(MockU2fDiscovery::StartSuccess));
  request.Start();

  // Add one U2F device
  auto device = std::make_unique<MockU2fDevice>();
  EXPECT_CALL(*device, TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  discovery_raw->DiscoverDevice(std::move(device),
                                U2fDiscovery::DeviceStatus::ADDED);

  EXPECT_EQ(U2fRequest::State::BUSY, request.state_);
}

}  // namespace device
