// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_hid_message.h"

#include <numeric>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"

namespace device {

// static
std::unique_ptr<FidoHidMessage> FidoHidMessage::Create(
    uint32_t channel_id,
    CtapHidDeviceCommand type,
    base::span<const uint8_t> data) {
  if (data.size() > kHidMaxMessageSize)
    return nullptr;

  if ((type == CtapHidDeviceCommand::kCtapHidMsg ||
       type == CtapHidDeviceCommand::kCtapHidCBOR) &&
      data.empty())
    return nullptr;

  if ((type == CtapHidDeviceCommand::kCtapHidCancel ||
       type == CtapHidDeviceCommand::kCtapHidWink) &&
      !data.empty())
    return nullptr;

  if (type == CtapHidDeviceCommand::kCtapHidLock && data.size() != 1 &&
      data[0] < kHidMaxLockSeconds)
    return nullptr;

  if (type == CtapHidDeviceCommand::kCtapHidInit && data.size() != 8)
    return nullptr;

  return std::unique_ptr<FidoHidMessage>(
      new FidoHidMessage(channel_id, type, data));
}

// static
std::unique_ptr<FidoHidMessage> FidoHidMessage::CreateFromSerializedData(
    base::span<const uint8_t> serialized_data) {
  size_t remaining_size = 0;
  if (serialized_data.size() > kHidPacketSize ||
      serialized_data.size() < kHidInitPacketHeaderSize)
    return nullptr;

  std::unique_ptr<FidoHidInitPacket> init_packet =
      FidoHidInitPacket::CreateFromSerializedData(serialized_data,
                                                  &remaining_size);

  if (init_packet == nullptr)
    return nullptr;

  return std::unique_ptr<FidoHidMessage>(
      new FidoHidMessage(std::move(init_packet), remaining_size));
}

FidoHidMessage::~FidoHidMessage() = default;

bool FidoHidMessage::MessageComplete() const {
  return remaining_size_ == 0;
}

std::vector<uint8_t> FidoHidMessage::GetMessagePayload() const {
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

std::vector<uint8_t> FidoHidMessage::PopNextPacket() {
  if (packets_.empty())
    return {};

  std::vector<uint8_t> data = packets_.front()->GetSerializedData();
  packets_.pop_front();
  return data;
}

bool FidoHidMessage::AddContinuationPacket(base::span<const uint8_t> buf) {
  size_t remaining_size = remaining_size_;
  auto cont_packet =
      FidoHidContinuationPacket::CreateFromSerializedData(buf, &remaining_size);

  // Reject packets with a different channel id
  if (!cont_packet || channel_id_ != cont_packet->channel_id())
    return false;

  remaining_size_ = remaining_size;
  packets_.push_back(std::move(cont_packet));
  return true;
}

size_t FidoHidMessage::NumPackets() const {
  return packets_.size();
}

FidoHidMessage::FidoHidMessage(uint32_t channel_id,
                               CtapHidDeviceCommand type,
                               base::span<const uint8_t> data)
    : channel_id_(channel_id) {
  size_t remaining_bytes = data.size();
  uint8_t sequence = 0;

  base::span<const uint8_t>::const_iterator first = data.cbegin();
  base::span<const uint8_t>::const_iterator last;

  if (remaining_bytes > kHidInitPacketDataSize) {
    last = first + kHidInitPacketDataSize;
    remaining_bytes -= kHidInitPacketDataSize;
  } else {
    last = first + remaining_bytes;
    remaining_bytes = 0;
  }

  packets_.push_back(std::make_unique<FidoHidInitPacket>(
      channel_id, type, std::vector<uint8_t>(first, last), data.size()));

  while (remaining_bytes > 0) {
    first = last;
    if (remaining_bytes > kHidContinuationPacketDataSize) {
      last = first + kHidContinuationPacketDataSize;
      remaining_bytes -= kHidContinuationPacketDataSize;
    } else {
      last = first + remaining_bytes;
      remaining_bytes = 0;
    }

    packets_.push_back(std::make_unique<FidoHidContinuationPacket>(
        channel_id, sequence, std::vector<uint8_t>(first, last)));
    sequence++;
  }
}

FidoHidMessage::FidoHidMessage(std::unique_ptr<FidoHidInitPacket> init_packet,
                               size_t remaining_size)
    : remaining_size_(remaining_size) {
  channel_id_ = init_packet->channel_id();
  cmd_ = static_cast<CtapHidDeviceCommand>(init_packet->command());
  packets_.push_back(std::move(init_packet));
}

}  // namespace device
