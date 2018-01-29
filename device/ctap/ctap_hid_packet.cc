// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_hid_packet.h"

#include <algorithm>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "device/ctap/ctap_constants.h"

namespace device {

CTAPHidPacket::CTAPHidPacket(const std::vector<uint8_t>& data,
                             uint32_t channel_id)
    : data_(data), channel_id_(channel_id) {}

CTAPHidPacket::CTAPHidPacket() = default;

CTAPHidPacket::~CTAPHidPacket() = default;

std::vector<uint8_t> CTAPHidPacket::GetPacketPayload() const {
  return data_;
}

// U2F Initialization packet is defined as:
// Offset Length
// 0      4       Channel ID
// 4      1       Command ID
// 5      1       High order packet payload size
// 6      1       Low order packet payload size
// 7      (s-7)   Payload data
CTAPHidInitPacket::CTAPHidInitPacket(uint32_t channel_id,
                                     uint8_t cmd,
                                     const std::vector<uint8_t>& data,
                                     uint16_t payload_length)
    : CTAPHidPacket(data, channel_id),
      command_(cmd),
      payload_length_(payload_length) {}

std::vector<uint8_t> CTAPHidInitPacket::GetSerializedData() {
  std::vector<uint8_t> serialized;
  serialized.reserve(kPacketSize);
  serialized.push_back((channel_id_ >> 24) & 0xff);
  serialized.push_back((channel_id_ >> 16) & 0xff);
  serialized.push_back((channel_id_ >> 8) & 0xff);
  serialized.push_back(channel_id_ & 0xff);
  serialized.push_back(command_);
  serialized.push_back((payload_length_ >> 8) & 0xff);
  serialized.push_back(payload_length_ & 0xff);
  serialized.insert(serialized.end(), data_.begin(), data_.end());
  serialized.resize(kPacketSize, 0);

  return serialized;
}

// static
std::unique_ptr<CTAPHidInitPacket> CTAPHidInitPacket::CreateFromSerializedData(
    const std::vector<uint8_t>& serialized,
    size_t* remaining_size) {
  if (remaining_size == nullptr || serialized.size() != kPacketSize)
    return nullptr;

  return std::make_unique<CTAPHidInitPacket>(serialized, remaining_size);
}

CTAPHidInitPacket::CTAPHidInitPacket(const std::vector<uint8_t>& serialized,
                                     size_t* remaining_size) {
  size_t index = 0;
  channel_id_ = (serialized[index++] & 0xff) << 24;
  channel_id_ |= (serialized[index++] & 0xff) << 16;
  channel_id_ |= (serialized[index++] & 0xff) << 8;
  channel_id_ |= serialized[index++] & 0xff;
  command_ = serialized[index++];

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

CTAPHidInitPacket::~CTAPHidInitPacket() = default;

// U2F Continuation packet is defined as:
// Offset Length
// 0      4       Channel ID
// 4      1       Packet sequence 0x00..0x7f
// 5      (s-5)   Payload data
CTAPHidContinuationPacket::CTAPHidContinuationPacket(
    const uint32_t channel_id,
    const uint8_t sequence,
    const std::vector<uint8_t>& data)
    : CTAPHidPacket(data, channel_id), sequence_(sequence) {}

std::vector<uint8_t> CTAPHidContinuationPacket::GetSerializedData() {
  std::vector<uint8_t> serialized;
  serialized.reserve(kPacketSize);
  serialized.push_back((channel_id_ >> 24) & 0xff);
  serialized.push_back((channel_id_ >> 16) & 0xff);
  serialized.push_back((channel_id_ >> 8) & 0xff);
  serialized.push_back(channel_id_ & 0xff);
  serialized.push_back(sequence_);
  serialized.insert(serialized.end(), data_.begin(), data_.end());
  serialized.resize(kPacketSize, 0);

  return serialized;
}

// static
std::unique_ptr<CTAPHidContinuationPacket>
CTAPHidContinuationPacket::CreateFromSerializedData(
    const std::vector<uint8_t>& serialized,
    size_t* remaining_size) {
  if (remaining_size == nullptr || serialized.size() != kPacketSize)
    return nullptr;

  return std::make_unique<CTAPHidContinuationPacket>(serialized,
                                                     remaining_size);
}

CTAPHidContinuationPacket::CTAPHidContinuationPacket(
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

CTAPHidContinuationPacket::~CTAPHidContinuationPacket() = default;

}  // namespace device
