// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_HID_PACKET_H_
#define DEVICE_U2F_U2F_HID_PACKET_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>
#include "base/containers/span.h"
#include "device/ctap/ctap_constants.h"

namespace device {

// HID Packets are defined by the specification at
// https://fidoalliance.org/specs/fido-v2.0-rd-20161004/fido-client-to-authenticator-protocol-v2.0-rd-20161004.html#message-and-packet-structure
// Packets are one of two types, initialization packets and continuation
// packets. HID Packets have header information and a payload. If a
// CTAPHidInitPacket cannot store the entire payload, further payload
// information is stored in HidContinuationPackets.
class U2FHidPacket {
 public:
  U2FHidPacket(std::vector<uint8_t> data, uint32_t channel_id);
  virtual ~U2FHidPacket();

  virtual std::vector<uint8_t> GetSerializedData() const = 0;
  const std::vector<uint8_t>& GetPacketPayload() const { return data_; }
  uint32_t channel_id() const { return channel_id_; }

 protected:
  U2FHidPacket();

  std::vector<uint8_t> data_;
  uint32_t channel_id_ = kHIDBroadcastChannel;

 private:
  friend class HidMessage;
};

// HidInitPacket, based on the CTAP specification consists of a header with data
// that is serialized into a IOBuffer. A channel identifier is allocated by the
// CTAP device to ensure its system-wide uniqueness. Command identifiers
// determine the type of message the packet corresponds to. Payload length
// is the length of the entire message payload, and the data is only the portion
// of the payload that will fit into the HidInitPacket.
class U2FHidInitPacket : public U2FHidPacket {
 public:
  // Creates a packet from the serialized data of an initialization packet. As
  // this is the first packet, the payload length of the entire message will be
  // included within the serialized data. Remaining size will be returned to
  // inform the callee how many additional packets to expect.
  static std::unique_ptr<U2FHidInitPacket> CreateFromSerializedData(
      base::span<const uint8_t> serialized,
      size_t* remaining_size);

  U2FHidInitPacket(uint32_t channel_id,
                   CTAPHIDDeviceCommand cmd,
                   std::vector<uint8_t> data,
                   uint16_t payload_length);
  ~U2FHidInitPacket() final;

  std::vector<uint8_t> GetSerializedData() const final;
  CTAPHIDDeviceCommand command() const { return command_; }
  uint16_t payload_length() const { return payload_length_; }

 private:
  CTAPHIDDeviceCommand command_;
  uint16_t payload_length_;
};

// HidContinuationPacket, based on the CTAP Specification consists of a header
// with data that is serialized into an IOBuffer. The channel identifier will
// be identical to the identifier in all other packets of the message. The
// packet sequence will be the sequence number of this particular packet, from
// 0x00 to 0x7f.
class U2FHidContinuationPacket : public U2FHidPacket {
 public:
  // Creates a packet from the serialized data of a continuation packet. As an
  // HidInitPacket would have arrived earlier with the total payload size,
  // the remaining size should be passed to inform the packet of how much data
  // to expect.
  static std::unique_ptr<U2FHidContinuationPacket> CreateFromSerializedData(
      base::span<const uint8_t> serialized,
      size_t* remaining_size);

  U2FHidContinuationPacket(uint32_t channel_id,
                           uint8_t sequence,
                           std::vector<uint8_t> data);
  ~U2FHidContinuationPacket() final;

  std::vector<uint8_t> GetSerializedData() const final;
  uint8_t sequence() const { return sequence_; }

 private:
  uint8_t sequence_;
};
}  // namespace device

#endif  // DEVICE_U2F_U2F_HID_PACKET_H_
