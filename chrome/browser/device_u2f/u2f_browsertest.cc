// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "base/debug/stack_trace.h"
#include "base/run_loop.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_filter.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluetooth_socket.h"
#include "device/bluetooth/bluetooth_uuid.h"

#include "base/message_loop/message_loop.h"
#include "base/task_runner.h"

#include "device/u2f/u2f_ble_connection.h"
#include "device/u2f/u2f_ble_device.h"
#include "device/u2f/u2f_ble_frames.h"

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

class U2fBrowserTest : public InProcessBrowserTest {
 public:
  U2fBrowserTest() {}
  ~U2fBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(U2fBrowserTest);
};

IN_PROC_BROWSER_TEST_F(U2fBrowserTest, U2fViaBluetooth) {
  base::RunLoop run_loop;
  auto quit = run_loop.QuitClosure();

  std::unique_ptr<U2fBleDevice> device(new U2fBleDevice("08:7C:BE:8A:0F:69"));

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
