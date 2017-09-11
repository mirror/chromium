// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_DEVICE_
#define DEVICE_U2F_U2F_BLE_DEVICE_

#include "device/u2f/u2f_device.h"

#include <list>
#include <string>

namespace device {

class U2fBleFrame;
class U2fBleConnection;

struct U2fBleTransaction;

// TODO(pkalinnikov): Timeouts.
class U2fBleDevice : public U2fDevice {
 public:
  using MessageCallback = base::Callback<void(std::vector<uint8_t>)>;

  U2fBleDevice(std::string address);
  ~U2fBleDevice() override;

  // U2fDevice:
  void TryWink(const WinkCallback& callback) override;
  std::string GetId() override;

  using PingCallback = base::Callback<void(std::vector<uint8_t>)>;
  void SendPing(std::vector<uint8_t> data, PingCallback);

 protected:
  // U2fDevice:
  void DeviceTransact(std::unique_ptr<U2fApduCommand> command,
                      const DeviceCallback& callback) override;
  base::WeakPtr<U2fDevice> GetWeakPtr() override;

 private:
  // INIT --> BUSY (reading control point length) --> READY <--> BUSY.
  // DEVICE_ERROR persists.
  enum class State { INIT, BUSY, READY, DEVICE_ERROR };

  void OnConnected(bool success);
  void OnControlPointLengthRead(uint16_t length);
  void OnMessage(std::vector<uint8_t> data);

  void SendFrame(U2fBleFrame frame, MessageCallback);
  void TrySend();
  void OnFragmentSent(bool success);

  State state_ = State::INIT;
  std::unique_ptr<U2fBleConnection> connection_;

  std::list<std::pair<U2fBleFrame, MessageCallback>> pending_frames_;
  std::unique_ptr<U2fBleTransaction> transaction_;

  size_t max_fragment_size_ = 0;
  std::vector<uint8_t> buffer_;

  base::WeakPtrFactory<U2fBleDevice> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fBleDevice);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_DEVICE_
