// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_device.h"
#include "device/u2f/u2f_apdu_command.h"
#include "device/u2f/u2f_ble_connection.h"
#include "device/u2f/u2f_ble_frames.h"

#include "base/bind.h"

#include <vector>

namespace device {

class U2fBleTransaction {
 public:
  U2fBleTransaction(U2fCommandType command,
                    std::vector<uint8_t> data,
                    size_t max_fragment_size)
      : frame_(command, std::move(data)) {
    std::tie(initial_, continuation_) = frame_.ToFragments(max_fragment_size);
    continuation_iter_ = continuation_.begin();
  }

  U2fCommandType command() const { return frame_.command(); }

  bool finished() const { return continuation_iter_ == continuation_.end(); }

  const U2fBleFrameInitializationFragment& initial_fragment() {
    return initial_;
  }

  const U2fBleFrameContinuationFragment& next_fragment() {
    DCHECK(!finished());
    return *continuation_iter_++;
  }

  U2fBleDevice::MessageCallback& callback() { return callback_; }

  std::unique_ptr<U2fBleFrameAssembler>& frame_assembler() {
    return frame_assembler_;
  }

 private:
  U2fBleFrame frame_;
  U2fBleFrameInitializationFragment initial_;
  std::vector<U2fBleFrameContinuationFragment> continuation_;
  std::vector<U2fBleFrameContinuationFragment>::iterator continuation_iter_;
  U2fBleDevice::MessageCallback callback_;
  std::unique_ptr<U2fBleFrameAssembler> frame_assembler_;
};

std::string U2fBleDevice::GetId(base::StringPiece address) {
  std::string result = "ble:";
  result.append(address.data(), address.size());
  return result;
}

U2fBleDevice::U2fBleDevice(std::string address) : weak_factory_(this) {
  connection_ = std::make_unique<U2fBleConnection>(
      std::move(address),
      base::BindOnce(&U2fBleDevice::OnConnected, weak_factory_.GetWeakPtr()),
      base::BindRepeating(&U2fBleDevice::OnMessage,
                          weak_factory_.GetWeakPtr()));
}

U2fBleDevice::~U2fBleDevice() = default;

void U2fBleDevice::TryWink(WinkCallback callback) {
  // U2F over BLE doesn't have the Wink command.
  std::move(callback).Run();
}

std::string U2fBleDevice::GetId() const {
  return "ble:" + connection_->address();
}

void U2fBleDevice::SendPing(std::vector<uint8_t> data, PingCallback callback) {
  U2fBleFrame frame(U2fCommandType::CMD_PING, std::move(data));
  SendFrame(std::move(frame),
            base::BindOnce(
                [](PingCallback callback, std::vector<uint8_t> response_data) {
                  std::move(callback).Run(std::move(response_data));
                },
                std::move(callback)));
}

void U2fBleDevice::DeviceTransact(std::unique_ptr<U2fApduCommand> command,
                                  DeviceCallback callback) {
  U2fBleFrame frame(U2fCommandType::CMD_MSG, command->GetEncodedCommand());
  SendFrame(
      std::move(frame),
      base::BindOnce(
          [](DeviceCallback callback, std::vector<uint8_t> response_data) {
            std::move(callback).Run(true, U2fApduResponse::CreateFromMessage(
                                              std::move(response_data)));
          },
          std::move(callback)));
}

base::WeakPtr<U2fDevice> U2fBleDevice::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void U2fBleDevice::OnConnected(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to connect to U2fBleDevice.";
    state_ = State::DEVICE_ERROR;
    return;
  }
  state_ = State::BUSY;
  connection_->ReadControlPointLength(base::BindOnce(
      &U2fBleDevice::OnControlPointLengthRead, weak_factory_.GetWeakPtr()));
}

void U2fBleDevice::OnControlPointLengthRead(uint16_t length) {
  if (!length) {
    LOG(ERROR) << "Failed to read Control Point Length.";
    state_ = State::DEVICE_ERROR;
    return;
  }
  max_fragment_size_ = static_cast<size_t>(length);
  buffer_.reserve(max_fragment_size_);

  state_ = State::READY;
  TrySend();
}

void U2fBleDevice::OnMessage(std::vector<uint8_t> data) {
  VLOG(2) << "Status value updated: size=" << data.size();

  DCHECK(transaction_);
  if (!transaction_->frame_assembler()) {
    U2fBleFrameInitializationFragment fragment;
    if (!U2fBleFrameInitializationFragment::Parse(data, &fragment)) {
      LOG(ERROR) << "Failed to parse initialization fragment.";
      return;
    }
    transaction_->frame_assembler() =
        std::make_unique<U2fBleFrameAssembler>(fragment);
  } else {
    U2fBleFrameContinuationFragment fragment;
    if (!U2fBleFrameContinuationFragment::Parse(data, &fragment)) {
      LOG(ERROR) << "Failed to parse continuation fragment.";
      return;
    }
    transaction_->frame_assembler()->AddFragment(fragment);
  }
  if (!transaction_->frame_assembler()->IsDone())
    return;

  U2fBleFrame frame = std::move(*transaction_->frame_assembler()->GetFrame());
  if (frame.command() == transaction_->command()) {
    std::move(transaction_->callback()).Run(std::move(frame.data()));
    transaction_ = nullptr;

    if (state_ != State::DEVICE_ERROR)
      state_ = State::READY;
    TrySend();
  } else if (frame.command() == U2fCommandType::CMD_KEEPALIVE) {
    VLOG(2) << "KEEPALIVE";
    transaction_->frame_assembler() = nullptr;
  } else {
    DCHECK_EQ(frame.command(), U2fCommandType::CMD_ERROR);
    state_ = State::DEVICE_ERROR;
  }
}

void U2fBleDevice::SendFrame(U2fBleFrame frame, MessageCallback callback) {
  if (state_ != State::READY) {
    pending_frames_.emplace(std::move(frame), std::move(callback));
    return;
  }
  state_ = State::BUSY;

  transaction_ = std::make_unique<U2fBleTransaction>(
      frame.command(), std::move(frame.data()), max_fragment_size_);
  transaction_->callback() = std::move(callback);

  buffer_.clear();
  transaction_->initial_fragment().Serialize(&buffer_);
  connection_->Write(buffer_, base::BindOnce(&U2fBleDevice::OnFragmentSent,
                                             weak_factory_.GetWeakPtr()));
}

void U2fBleDevice::OnFragmentSent(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to send frame fragment.";
    std::move(transaction_->callback()).Run({});  // FIXME
    return;
  }

  if (transaction_->finished())
    return;

  buffer_.clear();
  transaction_->next_fragment().Serialize(&buffer_);
  connection_->Write(buffer_, base::BindOnce(&U2fBleDevice::OnFragmentSent,
                                             weak_factory_.GetWeakPtr()));
}

void U2fBleDevice::TrySend() {
  if (state_ != State::READY || pending_frames_.empty())
    return;

  auto front_task = std::move(pending_frames_.front());
  pending_frames_.pop();
  SendFrame(std::move(front_task.first), std::move(front_task.second));
}

}  // namespace device
