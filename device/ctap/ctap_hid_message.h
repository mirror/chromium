// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_HID_MESSAGE_H_
#define DEVICE_CTAP_CTAP_HID_MESSAGE_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <vector>

#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_hid_packet.h"

namespace device {

// Represents HID message format defined by the specification at
// https://fidoalliance.org/specs/fido-v2.0-rd-20161004/fido-client-to-authenticator-protocol-v2.0-rd-20161004.html#message-and-packet-structure4
class CTAPHidMessage {
 public:
  // Static functions to create CTAP/U2F HID commands
  static std::unique_ptr<CTAPHidMessage> CreateHidMessageCmd(
      uint32_t channel_id,
      const std::vector<uint8_t>& data);
  static std::unique_ptr<CTAPHidMessage> CreateHidCBORCmd(
      uint32_t channel_id,
      const std::vector<uint8_t>& data);
  static std::unique_ptr<CTAPHidMessage> CreateHidInitCmd(
      const std::vector<uint8_t> nonce,
      uint32_t channel_id = kHIDBroadcastChannel);
  static std::unique_ptr<CTAPHidMessage> CreateHidPingCmd(
      uint32_t channel_id,
      const std::vector<uint8_t>& data);
  static std::unique_ptr<CTAPHidMessage> CreateHidWinkCmd(uint32_t channel_id);
  static std::unique_ptr<CTAPHidMessage> CreateHidLockCmd(uint32_t channel_id,
                                                          uint8_t lock_time);
  static std::unique_ptr<CTAPHidMessage> CreateHidCancelCmd(uint32_t channel_id,
                                                            uint8_t lock_time);
  static std::unique_ptr<CTAPHidMessage> CreateHidKeepAliveCmd(
      uint32_t channel_id,
      uint8_t lock_time);

  // Reconstruct a message from serialized message data
  static std::unique_ptr<CTAPHidMessage> CreateFromSerializedData(
      const std::vector<uint8_t>& buf);

  CTAPHidMessage(uint32_t channel_id,
                 CTAPHIDDeviceCommand type,
                 const std::vector<uint8_t>& data);
  CTAPHidMessage(std::unique_ptr<CTAPHidInitPacket> init_packet,
                 size_t remaining_size);
  ~CTAPHidMessage();

  bool MessageComplete() const;
  std::vector<uint8_t> GetMessagePayload() const;
  // Pop front of queue with next packet
  std::vector<uint8_t> PopNextPacket();
  // Adds a continuation packet to the packet list, from the serialized
  // response value
  bool AddContinuationPacket(const std::vector<uint8_t>& packet_buf);

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
  static std::unique_ptr<CTAPHidMessage> Create(
      uint32_t channel_id,
      CTAPHIDDeviceCommand cmd,
      const std::vector<uint8_t>& data);

  uint32_t channel_id_;
  CTAPHIDDeviceCommand cmd_;
  std::list<std::unique_ptr<CTAPHidPacket>> packets_;
  size_t remaining_size_;
};
}  // namespace device

#endif  // DEVICE_CTAP_CTAP_HID_MESSAGE_H_
