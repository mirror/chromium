// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_FIDO_HID_DEVICE_H_
#define DEVICE_CTAP_FIDO_HID_DEVICE_H_

#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/cancelable_callback.h"
#include "base/memory/weak_ptr.h"
#include "device/ctap/ctap_request_param.h"
#include "device/ctap/fido_device.h"
#include "device/ctap/hid_message.h"
#include "services/device/public/interfaces/hid.mojom.h"

namespace device {

class FidoHidDevice : public FidoDevice {
 public:
  using CTAPHidMessageCallback =
      base::OnceCallback<void(bool, std::unique_ptr<HidMessage>)>;
  using ConnectCallback = device::mojom::HidManager::ConnectCallback;
  using WinkCallback = base::OnceCallback<void()>;

  // Get a string identifier for a given device info
  static const std::string GetIdForDevice(
      const device::mojom::HidDeviceInfo& device_info);

  FidoHidDevice(device::mojom::HidDeviceInfoPtr device_info,
                device::mojom::HidManager* hid_manager);
  ~FidoHidDevice() override;

  // Send a wink command if supported
  void TryWink(WinkCallback callback);

  const std::string GetId() const override;

 private:
  void DeviceTransact(std::unique_ptr<CTAPRequestParam> command,
                      DeviceCallback callback) override;

  // Open a connection to this device
  void Connect(ConnectCallback callback);
  void OnConnect(std::unique_ptr<CTAPRequestParam> command,
                 DeviceCallback callback,
                 device::mojom::HidConnectionPtr connection);
  // Ask device to allocate a unique channel id for this connection
  void AllocateChannel(std::unique_ptr<CTAPRequestParam> command,
                       DeviceCallback callback);
  void OnAllocateChannel(std::vector<uint8_t> nonce,
                         std::unique_ptr<CTAPRequestParam> command,
                         DeviceCallback callback,
                         bool success,
                         std::unique_ptr<HidMessage> message);

  void Transition(std::unique_ptr<CTAPRequestParam> command,
                  DeviceCallback callback);
  // Write all message packets to device, and read response if expected
  void WriteMessage(std::unique_ptr<HidMessage> message,
                    bool response_expected,
                    CTAPHidMessageCallback callback);
  void PacketWritten(std::unique_ptr<HidMessage> message,
                     bool response_expected,
                     CTAPHidMessageCallback callback,
                     bool success);
  // Read all response message packets from device
  void ReadMessage(CTAPHidMessageCallback callback);
  void MessageReceived(DeviceCallback callback,
                       bool success,
                       std::unique_ptr<HidMessage> message);
  void OnRead(CTAPHidMessageCallback callback,
              bool success,
              uint8_t report_id,
              const base::Optional<std::vector<uint8_t>>& buf);
  void OnReadContinuation(std::unique_ptr<HidMessage> message,
                          CTAPHidMessageCallback callback,
                          bool success,
                          uint8_t report_id,
                          const base::Optional<std::vector<uint8_t>>& buf);
  void OnWink(WinkCallback callback,
              bool success,
              std::unique_ptr<HidMessage> response);
  void ArmTimeout(DeviceCallback callback);
  void OnTimeout(DeviceCallback callback);
  void KeepAliveCallback(DeviceCallback callback);

  base::WeakPtr<FidoDevice> GetWeakPtr() override;

  base::CancelableOnceClosure timeout_callback_;
  std::queue<std::pair<std::unique_ptr<CTAPRequestParam>, DeviceCallback>>
      pending_transactions_;
  uint32_t channel_id_;
  uint8_t capabilities_;
  device::mojom::HidManager* hid_manager_;
  device::mojom::HidDeviceInfoPtr device_info_;
  device::mojom::HidConnectionPtr connection_;

  base::WeakPtrFactory<FidoHidDevice> weak_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FidoHidDevice);
};

}  // namespace device

#endif  // DEVICE_CTAP_FIDO_HID_DEVICE_H_
