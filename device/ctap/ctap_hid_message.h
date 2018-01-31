// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_HID_MESSAGE_H_
#define DEVICE_CTAP_CTAP_HID_MESSAGE_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <vector>

#include "base/containers/span.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_hid_packet.h"

namespace device {

// Represents HID message format defined by the specification at
// https://fidoalliance.org/specs/fido-v2.0-rd-20161004/fido-client-to-authenticator-protocol-v2.0-rd-20161004.html#message-and-packet-structure4
class CTAPHidMessage {
 public:
  // Static functions to create CTAP/U2F HID commands
  static std::unique_ptr<CTAPHidMessage> Create(uint32_t channel_id,
                                                CTAPHIDDeviceCommand cmd,
                                                base::span<const uint8_t> data);

  // Reconstruct a message from serialized message data
  static std::unique_ptr<CTAPHidMessage> CreateFromSerializedData(
      base::span<const uint8_t> serialized_data);

  ~CTAPHidMessage();

  bool MessageComplete() const;
  std::vector<uint8_t> GetMessagePayload() const;
  // Pop front of queue with next packet
  std::vector<uint8_t> PopNextPacket();
  // Adds a continuation packet to the packet list, from the serialized
  // response value
  bool AddContinuationPacket(base::span<const uint8_t> packet_buf);

  size_t NumPackets() const;
  uint32_t channel_id() const { return channel_id_; }
  CTAPHIDDeviceCommand cmd() const { return cmd_; }
  std::list<std::unique_ptr<CTAPHidPacket>>::const_iterator begin() const {
    return packets_.cbegin();
  }
  std::list<std::unique_ptr<CTAPHidPacket>>::const_iterator end() const {
    return packets_.cend();
  }

 private:
  CTAPHidMessage(uint32_t channel_id,
                 CTAPHIDDeviceCommand type,
                 base::span<const uint8_t> data);
  CTAPHidMessage(std::unique_ptr<CTAPHidInitPacket> init_packet,
                 size_t remaining_size);
  uint32_t channel_id_ = kHIDBroadcastChannel;
  CTAPHIDDeviceCommand cmd_ = CTAPHIDDeviceCommand::kCtapHidMsg;
  std::list<std::unique_ptr<CTAPHidPacket>> packets_;
  size_t remaining_size_ = 0;
};
}  // namespace device

#endif  // DEVICE_CTAP_CTAP_HID_MESSAGE_H_
