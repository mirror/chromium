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

struct U2fBleTransaction {
  U2fBleFrame frame;
  U2fBleFrameInitializationFragment initial;
  std::vector<U2fBleFrameContinuationFragment> continuation;
  size_t fragment_to_be_sent = 0;
  U2fBleDevice::MessageCallback callback;
  std::unique_ptr<U2fBleFrameAssembler> frame_assembler;

  U2fBleTransaction(U2fCommandType command,
                    std::vector<uint8_t> data,
                    size_t max_fragment_size)
      : frame(command, std::move(data)), initial(command, 0, nullptr, 0) {
    auto fragments = frame.ToFragments(max_fragment_size);
    initial = fragments.first;
    continuation = std::move(fragments.second);
  }

  bool not_finished() const {
    return fragment_to_be_sent < continuation.size();
  }

  const U2fBleFrameContinuationFragment& next_fragment() {
    return continuation[fragment_to_be_sent++];
  }
};

U2fBleDevice::U2fBleDevice(std::string address) : weak_factory_(this) {
  connection_.reset(new U2fBleConnection(
      std::move(address),
      base::Bind(&U2fBleDevice::OnConnected, weak_factory_.GetWeakPtr()),
      base::Bind(&U2fBleDevice::OnMessage, weak_factory_.GetWeakPtr())));
}

U2fBleDevice::~U2fBleDevice() = default;

void U2fBleDevice::TryWink(const WinkCallback& callback) {
  // U2F over BLE doesn't have the Wink command.
}

std::string U2fBleDevice::GetId() {
  return "ble:" + connection_->address();
}

void U2fBleDevice::SendPing(std::vector<uint8_t> data, PingCallback callback) {
  U2fBleFrame frame(U2fCommandType::CMD_PING, std::move(data));
  SendFrame(std::move(frame),
            base::Bind(
                [](PingCallback callback, std::vector<uint8_t> response_data) {
                  callback.Run(response_data);
                },
                callback));
}

void U2fBleDevice::DeviceTransact(std::unique_ptr<U2fApduCommand> command,
                                  const DeviceCallback& callback) {
  U2fBleFrame frame(U2fCommandType::CMD_MSG, command->GetEncodedCommand());
  SendFrame(
      std::move(frame),
      base::Bind(
          [](DeviceCallback callback, std::vector<uint8_t> response_data) {
            callback.Run(true,
                         U2fApduResponse::CreateFromMessage(response_data));
          },
          callback));
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
  connection_->ReadControlPointLength(base::Bind(
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

  DCHECK(!!transaction_);
  if (!transaction_->frame_assembler) {
    U2fBleFrameInitializationFragment fragment;
    if (!U2fBleFrameInitializationFragment::Parse(data, &fragment)) {
      // TODO
    }
    transaction_->frame_assembler.reset(new U2fBleFrameAssembler(fragment));
  } else {
    U2fBleFrameContinuationFragment fragment;
    if (!U2fBleFrameContinuationFragment::Parse(data, &fragment)) {
      // TODO
    }
    transaction_->frame_assembler->AddFragment(fragment);
  }
  if (!transaction_->frame_assembler->IsDone())
    return;

  U2fBleFrame frame = std::move(*transaction_->frame_assembler->GetFrame());
  if (frame.command() == transaction_->frame.command()) {
    transaction_->callback.Run(std::move(std::move(frame.data())));
    transaction_.reset(nullptr);

    if (state_ != State::DEVICE_ERROR)
      state_ = State::READY;
    TrySend();
  } else if (frame.command() == U2fCommandType::CMD_KEEPALIVE) {
    VLOG(2) << "KEEPALIVE";
    transaction_->frame_assembler.reset(nullptr);
  } else {
    DCHECK_EQ(frame.command(), U2fCommandType::CMD_ERROR);
    state_ = State::DEVICE_ERROR;
  }
}

void U2fBleDevice::SendFrame(U2fBleFrame frame, MessageCallback callback) {
  if (state_ != State::READY) {
    pending_frames_.push_back(
        std::make_pair(std::move(frame), std::move(callback)));
    return;
  }
  state_ = State::BUSY;

  transaction_.reset(new U2fBleTransaction(
      frame.command(), std::move(frame.data()), max_fragment_size_));
  transaction_->callback = std::move(callback);

  buffer_.clear();
  transaction_->initial.Serialize(&buffer_);
  connection_->Write(buffer_, base::Bind(&U2fBleDevice::OnFragmentSent,
                                         weak_factory_.GetWeakPtr()));
}

void U2fBleDevice::OnFragmentSent(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to send frame fragment.";
    transaction_->callback.Run(std::vector<uint8_t>());  // FIXME
    return;
  }

  if (transaction_->not_finished()) {
    buffer_.clear();
    transaction_->next_fragment().Serialize(&buffer_);
    connection_->Write(buffer_, base::Bind(&U2fBleDevice::OnFragmentSent,
                                           weak_factory_.GetWeakPtr()));
    return;
  }
}

void U2fBleDevice::TrySend() {
  if (state_ != State::READY || pending_frames_.empty())
    return;

  auto front_task = std::move(pending_frames_.front());
  pending_frames_.pop_front();
  SendFrame(std::move(front_task.first), std::move(front_task.second));
}

}  // namespace device
