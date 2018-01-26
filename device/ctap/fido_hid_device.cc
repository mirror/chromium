// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/fido_hid_device.h"

#include <limits>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "crypto/random.h"
#include "device/ctap/ctap_constants.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace device {

FidoHidDevice::FidoHidDevice(device::mojom::HidDeviceInfoPtr device_info,
                             device::mojom::HidManager* hid_manager)
    : FidoDevice(),
      channel_id_(std::numeric_limits<uint32_t>::max()),
      hid_manager_(hid_manager),
      device_info_(std::move(device_info)),
      weak_factory_(this) {}

FidoHidDevice::~FidoHidDevice() = default;

void FidoHidDevice::DeviceTransact(std::unique_ptr<CTAPRequestParam> command,
                                   DeviceCallback callback) {
  // TODO(@hongjunchoi): add 200 ms delay for every repeating call to
  // DeviceTransact() to limit battery consumption.
  Transition(std::move(command), std::move(callback));
}

void FidoHidDevice::Transition(std::unique_ptr<CTAPRequestParam> command,
                               DeviceCallback callback) {
  // This adapter is needed to support the calls to ArmTimeout(). However, it is
  // still guaranteed that |callback| will only be invoked once.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  switch (state_) {
    case State::kInit:
      state_ = State::kBusy;
      ArmTimeout(repeating_callback);
      Connect(base::BindOnce(&FidoHidDevice::OnConnect,
                             weak_factory_.GetWeakPtr(), std::move(command),
                             repeating_callback));
      break;
    case State::kConnected:
      state_ = State::kBusy;
      ArmTimeout(repeating_callback);
      AllocateChannel(std::move(command), repeating_callback);
      break;

    case State::kReady: {
      state_ = State::kBusy;
      LOG(ERROR) << "\n\n writing to device \n\n";

      const auto& encoded_parameter = command->Encode();
      if (!encoded_parameter) {
        LOG(ERROR) << "Error in encoding input parameter";
        repeating_callback.Run(CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR,
                               std::vector<uint8_t>());
        return;
      }

      auto msg = SupportsCTAP() ? HidMessage::CreateHidCBORCmd(
                                      channel_id_, *encoded_parameter)
                                : HidMessage::CreateHidMessageCmd(
                                      channel_id_, *encoded_parameter);

      if (!msg) {
        LOG(ERROR) << "ERROR CONSTRUCTING MESSGE FROM INPUT PARAM";
        auto response_code =
            SupportsCTAP() ? CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR
                           : CTAPDeviceResponseCode::kCtap1ErrInvalidParameter;
        repeating_callback.Run(response_code, std::vector<uint8_t>());
        return;
      }

      // Write message to the device
      ArmTimeout(repeating_callback);
      WriteMessage(
          std::move(msg), true,
          base::BindOnce(&FidoHidDevice::MessageReceived,
                         weak_factory_.GetWeakPtr(), repeating_callback));
      break;
    }

    case State::kBusy:
      LOG(ERROR) << "\nBUSY QUEUING REQUEST\n";
      pending_transactions_.emplace(std::move(command), repeating_callback);
      break;

    case State::kDeviceError:
    default:
      base::WeakPtr<FidoHidDevice> self = weak_factory_.GetWeakPtr();
      repeating_callback.Run(GetDeviceProcessingError(),
                             std::vector<uint8_t>());
      // Executing callbacks may free |this|. Check |self| first.
      while (self && !pending_transactions_.empty()) {
        // Respond to any pending requests
        DeviceCallback pending_cb =
            std::move(pending_transactions_.front().second);
        pending_transactions_.pop();
        std::move(pending_cb)
            .Run(GetDeviceProcessingError(), std::vector<uint8_t>());
      }
      break;
  }
}

void FidoHidDevice::Connect(ConnectCallback callback) {
  DCHECK(hid_manager_);
  hid_manager_->Connect(device_info_->guid, std::move(callback));
}

void FidoHidDevice::OnConnect(std::unique_ptr<CTAPRequestParam> command,
                              DeviceCallback callback,
                              device::mojom::HidConnectionPtr connection) {
  if (state_ == State::kDeviceError)
    return;
  timeout_callback_.Cancel();

  if (connection) {
    connection_ = std::move(connection);
    state_ = State::kConnected;
  } else {
    state_ = State::kDeviceError;
  }
  Transition(std::move(command), std::move(callback));
}

void FidoHidDevice::AllocateChannel(std::unique_ptr<CTAPRequestParam> command,
                                    DeviceCallback callback) {
  // Send random nonce to device to verify received message
  std::vector<uint8_t> nonce(8);
  crypto::RandBytes(nonce.data(), nonce.size());

  std::unique_ptr<HidMessage> message =
      HidMessage::CreateHidInitCmd(nonce, kHIDBroadcastChannel);

  WriteMessage(std::move(message), true,
               base::BindOnce(&FidoHidDevice::OnAllocateChannel,
                              weak_factory_.GetWeakPtr(), nonce,
                              std::move(command), std::move(callback)));
}

void FidoHidDevice::OnAllocateChannel(std::vector<uint8_t> nonce,
                                      std::unique_ptr<CTAPRequestParam> command,
                                      DeviceCallback callback,
                                      bool success,
                                      std::unique_ptr<HidMessage> message) {
  if (state_ == State::kDeviceError)
    return;

  timeout_callback_.Cancel();

  if (!success || !message) {
    state_ = State::kDeviceError;
    LOG(ERROR) << "ERROR IN CHANNEL ALLOCATION CALLBACK";
    Transition(nullptr, std::move(callback));
    return;
  }

  // Channel allocation response is defined as:
  // 0: 8 byte nonce
  // 8: 4 byte channel id
  // 12: Protocol version id
  // 13: Major device version
  // 14: Minor device version
  // 15: Build device version
  // 16: Capabilities
  std::vector<uint8_t> payload = message->GetMessagePayload();
  if (payload.size() != 17) {
    state_ = State::kDeviceError;
    LOG(ERROR) << "ERROR IN CHANNEL ALLOCATION due to incorrect payload size";
    Transition(nullptr, std::move(callback));
    return;
  }

  std::vector<uint8_t> received_nonce(std::begin(payload),
                                      std::begin(payload) + 8);
  if (nonce != received_nonce) {
    state_ = State::kDeviceError;
    Transition(nullptr, std::move(callback));
    return;
  }

  size_t index = 8;
  channel_id_ = payload[index++] << 24;
  channel_id_ |= payload[index++] << 16;
  channel_id_ |= payload[index++] << 8;
  channel_id_ |= payload[index++];
  capabilities_ = payload[16];

  state_ = State::kReady;

  GetInfo(base::BindOnce(&FidoHidDevice::GetInfoCallback,
                         weak_factory_.GetWeakPtr(), std::move(command),
                         std::move(callback)));
}

void FidoHidDevice::WriteMessage(std::unique_ptr<HidMessage> message,
                                 bool response_expected,
                                 CTAPHidMessageCallback callback) {
  if (!connection_ || !message || message->NumPackets() == 0) {
    std::move(callback).Run(false, nullptr);
    return;
  }
  const auto& request_buffer = message->PopNextPacket();
  connection_->Write(
      kReportId, request_buffer,
      base::BindOnce(&FidoHidDevice::PacketWritten, weak_factory_.GetWeakPtr(),
                     std::move(message), true, std::move(callback)));
}

void FidoHidDevice::PacketWritten(std::unique_ptr<HidMessage> message,
                                  bool response_expected,
                                  CTAPHidMessageCallback callback,
                                  bool success) {
  if (success && message->NumPackets() > 0) {
    WriteMessage(std::move(message), response_expected, std::move(callback));
  } else if (success && response_expected) {
    ReadMessage(std::move(callback));
  } else {
    std::move(callback).Run(success, nullptr);
  }
}

void FidoHidDevice::ReadMessage(CTAPHidMessageCallback callback) {
  if (!connection_) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  connection_->Read(base::BindOnce(
      &FidoHidDevice::OnRead, weak_factory_.GetWeakPtr(), std::move(callback)));
}

void FidoHidDevice::OnRead(CTAPHidMessageCallback callback,
                           bool success,
                           uint8_t report_id,
                           const base::Optional<std::vector<uint8_t>>& buf) {
  if (!success) {
    std::move(callback).Run(success, nullptr);
    return;
  }

  DCHECK(buf);
  std::unique_ptr<HidMessage> read_message =
      HidMessage::CreateFromSerializedData(*buf);

  if (!read_message) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  // Received a message from a different channel, so try again
  if (channel_id_ != read_message->channel_id()) {
    connection_->Read(base::BindOnce(&FidoHidDevice::OnRead,
                                     weak_factory_.GetWeakPtr(),
                                     std::move(callback)));
    return;
  }

  if (read_message->MessageComplete()) {
    std::move(callback).Run(success, std::move(read_message));
    return;
  }

  // Continue reading additional packets
  connection_->Read(base::BindOnce(
      &FidoHidDevice::OnReadContinuation, weak_factory_.GetWeakPtr(),
      std::move(read_message), std::move(callback)));
}

void FidoHidDevice::OnReadContinuation(
    std::unique_ptr<HidMessage> message,
    CTAPHidMessageCallback callback,
    bool success,
    uint8_t report_id,
    const base::Optional<std::vector<uint8_t>>& buf) {
  if (!success) {
    std::move(callback).Run(success, nullptr);
    return;
  }

  DCHECK(buf);
  message->AddContinuationPacket(*buf);

  if (message->MessageComplete()) {
    std::move(callback).Run(success, std::move(message));
    return;
  }

  connection_->Read(base::BindOnce(&FidoHidDevice::OnReadContinuation,
                                   weak_factory_.GetWeakPtr(),
                                   std::move(message), std::move(callback)));
}

void FidoHidDevice::MessageReceived(DeviceCallback callback,
                                    bool success,
                                    std::unique_ptr<HidMessage> message) {
  if (state_ == State::kDeviceError)
    return;
  timeout_callback_.Cancel();

  if (!success) {
    LOG(ERROR) << "message received NOT SUCCESS";

    state_ = State::kDeviceError;
    Transition(nullptr, std::move(callback));
    return;
  }

  LOG(ERROR) << "received message command : "
             << static_cast<size_t>(message->cmd());

  if (message->cmd() == CTAPHIDDeviceCommand::kCtapHidKeepAlive) {
    LOG(ERROR) << "\nmessage received KEEP ALIVE\n";

    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&FidoHidDevice::KeepAliveCallback,
                       weak_factory_.GetWeakPtr(), std::move(callback)),
        kDelayKeepAlive);
    return;
  }

  base::WeakPtr<FidoHidDevice> self = weak_factory_.GetWeakPtr();

  // Depending on the specified protocol, decode the payload (bytes) into
  // pair of response code and data bytes
  auto response = DecodeDeviceResponse(message->GetMessagePayload());
  std::move(callback).Run(std::get<0>(response), std::get<1>(response));

  if (!self)
    return;

  state_ = State::kReady;

  // Executing |callback| may have freed |this|. Check |self| first.
  if (!pending_transactions_.empty()) {
    // If any transactions were queued, process the first one
    LOG(ERROR) << "\nPROCESSING QUEUED REQUESTS\n";
    std::unique_ptr<CTAPRequestParam> pending_cmd =
        std::move(pending_transactions_.front().first);
    DeviceCallback pending_cb = std::move(pending_transactions_.front().second);
    pending_transactions_.pop();
    Transition(std::move(pending_cmd), std::move(pending_cb));
  }
}

void FidoHidDevice::TryWink(WinkCallback callback) {
  // Only try to wink if device claims support
  if (!(capabilities_ & kWinkCapability) || state_ != State::kReady) {
    std::move(callback).Run();
    return;
  }

  std::unique_ptr<HidMessage> wink_message =
      HidMessage::CreateHidWinkCmd(channel_id_);
  WriteMessage(std::move(wink_message), true,
               base::BindOnce(&FidoHidDevice::OnWink,
                              weak_factory_.GetWeakPtr(), std::move(callback)));
}

void FidoHidDevice::OnWink(WinkCallback callback,
                           bool success,
                           std::unique_ptr<HidMessage> response) {
  std::move(callback).Run();
}

void FidoHidDevice::ArmTimeout(DeviceCallback callback) {
  DCHECK(timeout_callback_.IsCancelled());
  timeout_callback_.Reset(base::BindOnce(&FidoHidDevice::OnTimeout,
                                         weak_factory_.GetWeakPtr(),
                                         std::move(callback)));
  // Setup timeout task for 3 seconds
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, timeout_callback_.callback(),
      base::TimeDelta::FromMilliseconds(3000));
}

void FidoHidDevice::OnTimeout(DeviceCallback callback) {
  state_ = State::kDeviceError;
  Transition(nullptr, std::move(callback));
}

void FidoHidDevice::KeepAliveCallback(DeviceCallback callback) {
  LOG(ERROR) << "\n KEEP ALIVE MESSGE RECEIVED AND RE-READING \n ";
  ReadMessage(base::BindOnce(&FidoHidDevice::MessageReceived,
                             weak_factory_.GetWeakPtr(), std::move(callback)));
}

// static
const std::string FidoHidDevice::GetIdForDevice(
    const device::mojom::HidDeviceInfo& device_info) {
  return "hid:" + device_info.guid;
}

const std::string FidoHidDevice::GetId() const {
  return GetIdForDevice(*device_info_);
}

base::WeakPtr<FidoDevice> FidoHidDevice::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace device
