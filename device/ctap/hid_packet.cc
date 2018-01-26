// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/hid_packet.h"

#include <cstring>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "device/ctap/ctap_constants.h"

namespace device {

HidPacket::HidPacket(const std::vector<uint8_t>& data, uint32_t channel_id)
    : data_(data), channel_id_(channel_id) {}

HidPacket::HidPacket() = default;

HidPacket::~HidPacket() = default;

// static
std::unique_ptr<HidInitPacket> HidInitPacket::CreateFromSerializedData(
    const std::vector<uint8_t>& serialized,
    size_t* remaining_size) {
  if (remaining_size == nullptr || serialized.size() != kPacketSize)
    return nullptr;

  return std::make_unique<HidInitPacket>(serialized, remaining_size);
}

// U2F Initialization packet is defined as:
// Offset Length
// 0      4       Channel ID
// 4      1       Command ID
// 5      1       High order packet payload size
// 6      1       Low order packet payload size
// 7      (s-7)   Payload data
HidInitPacket::HidInitPacket(uint32_t channel_id,
                             uint8_t cmd,
                             const std::vector<uint8_t>& data,
                             uint16_t payload_length)
    : HidPacket(data, channel_id),
      command_(cmd),
      payload_length_(payload_length) {}

HidInitPacket::HidInitPacket(const std::vector<uint8_t>& serialized,
                             size_t* remaining_size) {
  size_t index = 0;
  channel_id_ = (serialized[index++] & 0xff) << 24;
  channel_id_ |= (serialized[index++] & 0xff) << 16;
  channel_id_ |= (serialized[index++] & 0xff) << 8;
  channel_id_ |= serialized[index++] & 0xff;

  command_ = serialized[index++] & 0x7f;

  uint16_t payload_size = serialized[index++] << 8;
  payload_size |= serialized[index++];
  payload_length_ = payload_size;

  // Check to see if payload is less than maximum size and padded with 0s
  uint16_t data_size =
      std::min(payload_size, static_cast<uint16_t>(kPacketSize - index));
  // Update remaining size to determine the payload size of follow on packets
  *remaining_size = payload_size - data_size;

  data_.insert(data_.end(), serialized.begin() + index,
               serialized.begin() + index + data_size);
}

HidInitPacket::~HidInitPacket() = default;

std::vector<uint8_t> HidInitPacket::GetSerializedData() {
  std::vector<uint8_t> serialized;
  serialized.reserve(kPacketSize);
  serialized.push_back((channel_id_ >> 24) & 0xff);
  serialized.push_back((channel_id_ >> 16) & 0xff);
  serialized.push_back((channel_id_ >> 8) & 0xff);
  serialized.push_back(channel_id_ & 0xff);
  serialized.push_back(command_ | 0x80);
  serialized.push_back((payload_length_ >> 8) & 0xff);
  serialized.push_back(payload_length_ & 0xff);
  serialized.insert(serialized.end(), data_.begin(), data_.end());
  serialized.resize(kPacketSize, 0);

  return serialized;
}

std::vector<uint8_t> HidPacket::GetPacketPayload() const {
  return data_;
}

// U2F Continuation packet is defined as:
// Offset Length
// 0      4       Channel ID
// 4      1       Packet sequence 0x00..0x7f
// 5      (s-5)   Payload data
HidContinuationPacket::HidContinuationPacket(const uint32_t channel_id,
                                             const uint8_t sequence,
                                             const std::vector<uint8_t>& data)
    : HidPacket(data, channel_id), sequence_(sequence) {}

std::vector<uint8_t> HidContinuationPacket::GetSerializedData() {
  std::vector<uint8_t> serialized;
  serialized.reserve(kPacketSize);
  serialized.push_back((channel_id_ >> 24) & 0xff);
  serialized.push_back((channel_id_ >> 16) & 0xff);
  serialized.push_back((channel_id_ >> 8) & 0xff);
  serialized.push_back(channel_id_ & 0xff);
  serialized.push_back(sequence_ & 0x7f);
  serialized.insert(serialized.end(), data_.begin(), data_.end());
  serialized.resize(kPacketSize, 0);

  return serialized;
}

// static
std::unique_ptr<HidContinuationPacket>
HidContinuationPacket::CreateFromSerializedData(
    const std::vector<uint8_t>& serialized,
    size_t* remaining_size) {
  if (remaining_size == nullptr || serialized.size() != kPacketSize)
    return nullptr;

  return std::make_unique<HidContinuationPacket>(serialized, remaining_size);
}

HidContinuationPacket::HidContinuationPacket(
    const std::vector<uint8_t>& serialized,
    size_t* remaining_size) {
  size_t index = 0;
  channel_id_ = (serialized[index++] & 0xff) << 24;
  channel_id_ |= (serialized[index++] & 0xff) << 16;
  channel_id_ |= (serialized[index++] & 0xff) << 8;
  channel_id_ |= serialized[index++] & 0xff;
  sequence_ = serialized[index++];

  // Check to see if packet payload is less than maximum size and padded with 0s
  size_t data_size = std::min(*remaining_size, kPacketSize - index);
  *remaining_size -= data_size;
  data_.insert(data_.end(), serialized.begin() + index,
               serialized.begin() + index + data_size);
}

HidContinuationPacket::~HidContinuationPacket() = default;

}  // namespace device
