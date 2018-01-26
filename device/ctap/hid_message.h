// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_HID_MESSAGE_H_
#define DEVICE_CTAP_HID_MESSAGE_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <vector>

#include "device/ctap/ctap_constants.h"
#include "device/ctap/hid_packet.h"

namespace device {

// Represents HID message format defined by the specification at
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-
// authenticator-protocol-v2.0-rd-20170927.html#authenticatorgetinfo-0x04
class HidMessage {
 public:
  // Static functions to create CTAP/U2F HID commands
  static std::unique_ptr<HidMessage> CreateHidMessageCmd(
      uint32_t channel_id,
      const std::vector<uint8_t>& data);
  static std::unique_ptr<HidMessage> CreateHidCBORCmd(
      uint32_t channel_id,
      const std::vector<uint8_t>& data);
  static std::unique_ptr<HidMessage> CreateHidInitCmd(
      const std::vector<uint8_t>& nonce,
      uint32_t channel_id = kHIDBroadcastChannel);
  static std::unique_ptr<HidMessage> CreateHidPingCmd(uint32_t channel_id);
  static std::unique_ptr<HidMessage> CreateHidWinkCmd(uint32_t channel_id);
  static std::unique_ptr<HidMessage> CreateHidLockCmd(uint32_t channel_id,
                                                      uint8_t lock_time);
  static std::unique_ptr<HidMessage> CreateHidCancelCmd(uint32_t channel_id,
                                                        uint8_t lock_time);
  static std::unique_ptr<HidMessage> CreateHidKeepAliveCmd(uint32_t channel_id,
                                                           uint8_t lock_time);

  // Reconstruct a message from serialized message data
  static std::unique_ptr<HidMessage> CreateFromSerializedData(
      const std::vector<uint8_t>& buf);

  HidMessage(uint32_t channel_id,
             CTAPHIDDeviceCommand type,
             const std::vector<uint8_t>& data);
  HidMessage(std::unique_ptr<HidInitPacket> init_packet, size_t remaining_size);
  ~HidMessage();

  bool MessageComplete() const;
  std::vector<uint8_t> GetMessagePayload() const;
  // Pop front of queue with next packet
  std::vector<uint8_t> PopNextPacket();
  // Adds a continuation packet to the packet list, from the serialized
  // response value
  bool AddContinuationPacket(const std::vector<uint8_t>& packet_buf);
  size_t NumPackets() const;

  uint32_t channel_id() { return channel_id_; }
  CTAPHIDDeviceCommand cmd() const { return cmd_; }
  std::list<std::unique_ptr<HidPacket>>::const_iterator begin() const {
    return packets_.cbegin();
  }
  std::list<std::unique_ptr<HidPacket>>::const_iterator end() const {
    return packets_.cend();
  }

 private:
  static std::unique_ptr<HidMessage> Create(uint32_t channel_id,
                                            CTAPHIDDeviceCommand cmd,
                                            const std::vector<uint8_t>& data);

  CTAPHIDDeviceCommand cmd_;
  std::list<std::unique_ptr<HidPacket>> packets_;
  size_t remaining_size_;
  uint32_t channel_id_;
};
}  // namespace device

#endif  // DEVICE_CTAP_HID_MESSAGE_H_
