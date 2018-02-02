// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_HID_MESSAGE_H_
#define DEVICE_U2F_U2F_HID_MESSAGE_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/containers/queue.h"
#include "base/containers/span.h"
#include "device/ctap/ctap_constants.h"
#include "device/u2f/u2f_hid_packet.h"

namespace device {

// Represents HID message format defined by the specification at
// https://fidoalliance.org/specs/fido-v2.0-rd-20161004/fido-client-to-authenticator-protocol-v2.0-rd-20161004.html#message-and-packet-structure4
class U2FHidMessage {
 public:
  // Static functions to create CTAP/U2F HID commands
  static std::unique_ptr<U2FHidMessage> Create(uint32_t channel_id,
                                               CTAPHIDDeviceCommand cmd,
                                               base::span<const uint8_t> data);

  // Reconstruct a message from serialized message data
  static std::unique_ptr<U2FHidMessage> CreateFromSerializedData(
      base::span<const uint8_t> serialized_data);

  ~U2FHidMessage();

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
  const base::circular_deque<std::unique_ptr<U2FHidPacket>>&
  GetPacketsForTesting() const {
    return packets_;
  }

 private:
  U2FHidMessage(uint32_t channel_id,
                CTAPHIDDeviceCommand type,
                base::span<const uint8_t> data);
  U2FHidMessage(std::unique_ptr<U2FHidInitPacket> init_packet,
                size_t remaining_size);
  uint32_t channel_id_ = kHIDBroadcastChannel;
  CTAPHIDDeviceCommand cmd_ = CTAPHIDDeviceCommand::kCtapHidMsg;
  base::circular_deque<std::unique_ptr<U2FHidPacket>> packets_;
  size_t remaining_size_ = 0;
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_HID_MESSAGE_H_
