// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_hid_message.h"

#include <numeric>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"

namespace device {

// static
std::unique_ptr<U2FHidMessage> U2FHidMessage::Create(
    uint32_t channel_id,
    CTAPHIDDeviceCommand type,
    base::span<const uint8_t> data) {
  if (data.size() > kHIDMaxMessageSize)
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

  return std::unique_ptr<U2FHidMessage>(
      new U2FHidMessage(channel_id, type, data));
}

// static
std::unique_ptr<U2FHidMessage> U2FHidMessage::CreateFromSerializedData(
    base::span<const uint8_t> serialized_data) {
  size_t remaining_size = 0;
  if (serialized_data.size() > kHIDPacketSize ||
      serialized_data.size() < kHIDInitPacketHeaderSize)
    return nullptr;

  std::unique_ptr<U2FHidInitPacket> init_packet =
      U2FHidInitPacket::CreateFromSerializedData(serialized_data,
                                                 &remaining_size);

  if (init_packet == nullptr)
    return nullptr;

  return std::unique_ptr<U2FHidMessage>(
      new U2FHidMessage(std::move(init_packet), remaining_size));
}

U2FHidMessage::~U2FHidMessage() = default;

bool U2FHidMessage::MessageComplete() const {
  return remaining_size_ == 0;
}

std::vector<uint8_t> U2FHidMessage::GetMessagePayload() const {
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

std::vector<uint8_t> U2FHidMessage::PopNextPacket() {
  if (packets_.empty())
    return {};

  std::vector<uint8_t> data = packets_.front()->GetSerializedData();
  packets_.pop_front();
  return data;
}

bool U2FHidMessage::AddContinuationPacket(base::span<const uint8_t> buf) {
  size_t remaining_size = remaining_size_;
  auto cont_packet =
      U2FHidContinuationPacket::CreateFromSerializedData(buf, &remaining_size);

  // Reject packets with a different channel id
  if (!cont_packet || channel_id_ != cont_packet->channel_id())
    return false;

  remaining_size_ = remaining_size;
  packets_.push_back(std::move(cont_packet));
  return true;
}

size_t U2FHidMessage::NumPackets() const {
  return packets_.size();
}

U2FHidMessage::U2FHidMessage(uint32_t channel_id,
                             CTAPHIDDeviceCommand type,
                             base::span<const uint8_t> data)
    : channel_id_(channel_id) {
  size_t remaining_bytes = data.size();
  uint8_t sequence = 0;

  base::span<const uint8_t>::const_iterator first = data.cbegin();
  base::span<const uint8_t>::const_iterator last;

  if (remaining_bytes > kHIDInitPacketDataSize) {
    last = first + kHIDInitPacketDataSize;
    remaining_bytes -= kHIDInitPacketDataSize;
  } else {
    last = first + remaining_bytes;
    remaining_bytes = 0;
  }

  packets_.push_back(std::make_unique<U2FHidInitPacket>(
      channel_id, type, std::vector<uint8_t>(first, last), data.size()));

  while (remaining_bytes > 0) {
    first = last;
    if (remaining_bytes > kHIDContinuationPacketDataSize) {
      last = first + kHIDContinuationPacketDataSize;
      remaining_bytes -= kHIDContinuationPacketDataSize;
    } else {
      last = first + remaining_bytes;
      remaining_bytes = 0;
    }

    packets_.push_back(std::make_unique<U2FHidContinuationPacket>(
        channel_id, sequence, std::vector<uint8_t>(first, last)));
    sequence++;
  }
}

U2FHidMessage::U2FHidMessage(std::unique_ptr<U2FHidInitPacket> init_packet,
                             size_t remaining_size)
    : remaining_size_(remaining_size) {
  channel_id_ = init_packet->channel_id();
  cmd_ = static_cast<CTAPHIDDeviceCommand>(init_packet->command());
  packets_.push_back(std::move(init_packet));
}

}  // namespace device
