// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_hid_message.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"

namespace device {

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidMessageCmd(
    uint32_t channel_id,
    const std::vector<uint8_t>& data) {
  return Create(channel_id, CTAPHIDDeviceCommand::kCtapHidMsg, data);
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidCBORCmd(
    uint32_t channel_id,
    const std::vector<uint8_t>& data) {
  return Create(channel_id, CTAPHIDDeviceCommand::kCtapHidCBOR, data);
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidInitCmd(
    const std::vector<uint8_t> nonce,
    uint32_t channel_id) {
  return Create(channel_id, CTAPHIDDeviceCommand::kCtapHidInit, nonce);
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidPingCmd(
    uint32_t channel_id,
    const std::vector<uint8_t>& data) {
  return Create(channel_id, CTAPHIDDeviceCommand::kCtapHidPing, data);
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidWinkCmd(
    uint32_t channel_id) {
  return Create(channel_id, CTAPHIDDeviceCommand::kCtapHidWink,
                std::vector<uint8_t>());
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidLockCmd(
    uint32_t channel_id,
    uint8_t lock_time) {
  return lock_time > kMaxHIDLockSeconds
             ? nullptr
             : Create(channel_id, CTAPHIDDeviceCommand::kCtapHidLock,
                      std::vector<uint8_t>{lock_time});
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateHidCancelCmd(
    uint32_t channel_id,
    uint8_t lock_time) {
  return Create(channel_id, CTAPHIDDeviceCommand::kCtapHidCancel,
                std::vector<uint8_t>());
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateFromSerializedData(
    const std::vector<uint8_t>& buf) {
  size_t remaining_size = 0;
  if (buf.size() > kPacketSize || buf.size() < kInitPacketHeader)
    return nullptr;

  std::unique_ptr<CTAPHidInitPacket> init_packet =
      CTAPHidInitPacket::CreateFromSerializedData(buf, &remaining_size);
  if (init_packet == nullptr)
    return nullptr;
  return std::make_unique<CTAPHidMessage>(std::move(init_packet),
                                          remaining_size);
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::Create(
    uint32_t channel_id,
    CTAPHIDDeviceCommand type,
    const std::vector<uint8_t>& data) {
  if (data.size() > kMaxMessageSize)
    return nullptr;
  return std::make_unique<CTAPHidMessage>(channel_id, type, data);
}

CTAPHidMessage::CTAPHidMessage(std::unique_ptr<CTAPHidInitPacket> init_packet,
                               size_t remaining_size)
    : remaining_size_(remaining_size) {
  channel_id_ = init_packet->channel_id();
  cmd_ = static_cast<CTAPHIDDeviceCommand>(init_packet->command());
  packets_.push_back(std::move(init_packet));
}

CTAPHidMessage::CTAPHidMessage(uint32_t channel_id,
                               CTAPHIDDeviceCommand type,
                               const std::vector<uint8_t>& data)
    : packets_(), remaining_size_(), channel_id_(channel_id) {
  size_t remaining_bytes = data.size();
  uint8_t sequence = 0;

  std::vector<uint8_t>::const_iterator first = data.begin();
  std::vector<uint8_t>::const_iterator last;

  if (remaining_bytes > kInitPacketDataSize) {
    last = data.begin() + kInitPacketDataSize;
    remaining_bytes -= kInitPacketDataSize;
  } else {
    last = data.begin() + remaining_bytes;
    remaining_bytes = 0;
  }

  packets_.push_back(std::make_unique<CTAPHidInitPacket>(
      channel_id, base::strict_cast<uint8_t>(type),
      std::vector<uint8_t>(first, last), data.size()));

  while (remaining_bytes > 0) {
    first = last;
    if (remaining_bytes > kContinuationPacketDataSize) {
      last = first + kContinuationPacketDataSize;
      remaining_bytes -= kContinuationPacketDataSize;
    } else {
      last = first + remaining_bytes;
      remaining_bytes = 0;
    }

    packets_.push_back(std::make_unique<CTAPHidContinuationPacket>(
        channel_id, sequence, std::vector<uint8_t>(first, last)));
    sequence++;
  }
}

CTAPHidMessage::~CTAPHidMessage() = default;

std::vector<uint8_t> CTAPHidMessage::PopNextPacket() {
  std::vector<uint8_t> data;
  if (NumPackets() > 0) {
    data = packets_.front()->GetSerializedData();
    packets_.pop_front();
  }
  return data;
}

bool CTAPHidMessage::AddContinuationPacket(const std::vector<uint8_t>& buf) {
  size_t remaining_size = remaining_size_;
  std::unique_ptr<CTAPHidContinuationPacket> cont_packet =
      CTAPHidContinuationPacket::CreateFromSerializedData(buf, &remaining_size);

  // Reject packets with a different channel id
  if (cont_packet == nullptr || channel_id_ != cont_packet->channel_id())
    return false;

  remaining_size_ = remaining_size;
  packets_.push_back(std::move(cont_packet));
  return true;
}

bool CTAPHidMessage::MessageComplete() const {
  return remaining_size_ == 0;
}

std::vector<uint8_t> CTAPHidMessage::GetMessagePayload() const {
  std::vector<uint8_t> data;

  for (const auto& packet : packets_) {
    std::vector<uint8_t> packet_data = packet->GetPacketPayload();
    data.insert(std::end(data), packet_data.cbegin(), packet_data.cend());
  }

  return data;
}

size_t CTAPHidMessage::NumPackets() const {
  return packets_.size();
}

}  // namespace device
