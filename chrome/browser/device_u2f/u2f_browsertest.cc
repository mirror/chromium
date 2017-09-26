// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "base/debug/stack_trace.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/task_runner.h"

#include "device/u2f/u2f_ble_connection.h"
#include "device/u2f/u2f_ble_device.h"
#include "device/u2f/u2f_ble_discovery.h"
#include "device/u2f/u2f_ble_frames.h"

#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

namespace {

char HexToChar(uint8_t hex) {
  return hex < 10 ? '0' + hex : 'A' + (hex - 10);
}

std::string Format(const std::vector<uint8_t>& data) {
  std::string result;
  for (uint8_t byte : data) {
    result.push_back(HexToChar((byte >> 4) & 0xF));
    result.push_back(HexToChar(byte & 0xF));
    result.push_back(' ');
  }
  return result;
}

}  // namespace

namespace device {

namespace {

// FIXME: This is duplicated in u2f_ble_unittest.
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

// FIXME: This is duplicated in u2f_ble_unittest.
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

}  // namespace

class U2fBrowserTest : public InProcessBrowserTest {
 public:
  U2fBrowserTest() {}
  ~U2fBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(U2fBrowserTest);
};

IN_PROC_BROWSER_TEST_F(U2fBrowserTest, U2fViaBluetooth) {
  DiscoveryCallbackReceiver receiver;
  DiscoveryStartedCallbackReceiver started_receiver;

  U2fBleDiscovery discovery;
  discovery.Start(started_receiver.GetCallback(), receiver.GetCallback());
  started_receiver.WaitForStarted();
  started_receiver.ExpectReceivedOnce(true);

  receiver.WaitForDiscovery();
  discovery.Stop();

  const std::set<std::string>& addresses = discovery.devices();
  if (addresses.empty()) {
    return;
  }
  for (const auto& address : addresses) {
    LOG(INFO) << "Found device: " << address;
  }

  // ===========================================================================

  base::RunLoop run_loop;
  auto quit = run_loop.QuitClosure();

  LOG(INFO) << "Using device: " << *addresses.begin();
  std::unique_ptr<U2fBleDevice> device(new U2fBleDevice(*addresses.begin()));

  device->SendPing(
      std::vector<uint8_t>(),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(1, 'a'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(10, 'b'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(20, 'c'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(567, 'd'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->Register(std::vector<uint8_t>(32), std::vector<uint8_t>(32),
                   base::Bind(
                       [](base::Closure quit, U2fReturnCode code,
                          const std::vector<uint8_t>& data) {
                         LOG(INFO)
                             << "code(" << int(code) << ") " << Format(data);
                         quit.Run();
                       },
                       quit));

  run_loop.Run();
}

}  // namespace device
