// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_hid_message.h"

#include <numeric>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"

namespace device {

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::Create(
    uint32_t channel_id,
    CTAPHIDDeviceCommand type,
    base::span<const uint8_t> data) {
  if (data.size() > kMaxMessageSize)
    return nullptr;

  if ((type == CTAPHIDDeviceCommand::kCtapHidMsg ||
       type == CTAPHIDDeviceCommand::kCtapHidCBOR) &&
      data.empty())
    return nullptr;

  if ((type == CTAPHIDDeviceCommand::kCtapHidCancel ||
       type == CTAPHIDDeviceCommand::kCtapHidWink) &&
      !data.empty())
    return nullptr;

  if (type == CTAPHIDDeviceCommand::kCtapHidLock && data.size() != 1 &&
      data[0] < kMaxHIDLockSeconds)
    return nullptr;

  if (type == CTAPHIDDeviceCommand::kCtapHidInit && data.size() != 8)
    return nullptr;

  return std::unique_ptr<CTAPHidMessage>(
      new CTAPHidMessage(channel_id, type, data));
}

// static
std::unique_ptr<CTAPHidMessage> CTAPHidMessage::CreateFromSerializedData(
    base::span<const uint8_t> serialized_data) {
  size_t remaining_size = 0;
  if (serialized_data.size() > kPacketSize ||
      serialized_data.size() < kInitPacketHeader)
    return nullptr;

  std::unique_ptr<CTAPHidInitPacket> init_packet =
      CTAPHidInitPacket::CreateFromSerializedData(serialized_data,
                                                  &remaining_size);

  if (init_packet == nullptr)
    return nullptr;

  return std::unique_ptr<CTAPHidMessage>(
      new CTAPHidMessage(std::move(init_packet), remaining_size));
}

CTAPHidMessage::~CTAPHidMessage() = default;

bool CTAPHidMessage::MessageComplete() const {
  return remaining_size_ == 0;
}

std::vector<uint8_t> CTAPHidMessage::GetMessagePayload() const {
  std::vector<uint8_t> data;
  size_t data_size = 0;
  for (const auto& packet : packets_) {
    data_size += packet->GetPacketPayload().size();
  }
  data.reserve(data_size);

  for (const auto& packet : packets_) {
    const auto& packet_data = packet->GetPacketPayload();
    data.insert(std::end(data), packet_data.cbegin(), packet_data.cend());
  }

  return data;
}

std::vector<uint8_t> CTAPHidMessage::PopNextPacket() {
  if (packets_.empty())
    return {};

  std::vector<uint8_t> data = packets_.front()->GetSerializedData();
  packets_.pop_front();
  return data;
}

bool CTAPHidMessage::AddContinuationPacket(base::span<const uint8_t> buf) {
  size_t remaining_size = remaining_size_;
  auto cont_packet =
      CTAPHidContinuationPacket::CreateFromSerializedData(buf, &remaining_size);

  // Reject packets with a different channel id
  if (!cont_packet || channel_id_ != cont_packet->channel_id())
    return false;

  remaining_size_ = remaining_size;
  packets_.push_back(std::move(cont_packet));
  return true;
}

size_t CTAPHidMessage::NumPackets() const {
  return packets_.size();
}

CTAPHidMessage::CTAPHidMessage(uint32_t channel_id,
                               CTAPHIDDeviceCommand type,
                               base::span<const uint8_t> data)
    : channel_id_(channel_id) {
  size_t remaining_bytes = data.size();
  uint8_t sequence = 0;

  base::span<const uint8_t>::const_iterator first = data.cbegin();
  base::span<const uint8_t>::const_iterator last;

  if (remaining_bytes > kInitPacketDataSize) {
    last = first + kInitPacketDataSize;
    remaining_bytes -= kInitPacketDataSize;
  } else {
    last = first + remaining_bytes;
    remaining_bytes = 0;
  }

  packets_.push_back(std::make_unique<CTAPHidInitPacket>(
      channel_id, type, std::vector<uint8_t>(first, last), data.size()));

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

CTAPHidMessage::CTAPHidMessage(std::unique_ptr<CTAPHidInitPacket> init_packet,
                               size_t remaining_size)
    : remaining_size_(remaining_size) {
  channel_id_ = init_packet->channel_id();
  cmd_ = static_cast<CTAPHIDDeviceCommand>(init_packet->command());
  packets_.push_back(std::move(init_packet));
}

}  // namespace device
