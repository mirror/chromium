// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_DEVICE_COMMAND_H_
#define DEVICE_CTAP_CTAP_DEVICE_COMMAND_H_

#include <stdint.h>

namespace device {

enum class CTAPHIDDeviceCommand : uint8_t {
  CTAPHID_MSG = 0x03,
  CTAPHID_CBOR = 0x10,
  CTAPHID_INIT = 0x06,
  CTAPHID_PING = 0x01,
  CTAPHID_CANCEL = 0x11,
  CTAPHID_ERROR = 0x3F,
  CTAPHID_KEEPALIVE = 0x3B,
  CTAPHID_WINK = 0x08,
  CTAPHID_LOCK = 0x04,
};

enum class CTAPNFCDeviceCommand : uint8_t {

};

enum class CTAPBLEDeviceCommand : uint8_t {

};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_DEVICE_COMMAND_H_
