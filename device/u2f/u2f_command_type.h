// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_COMMAND_TYPE_H_
#define DEVICE_U2F_U2F_COMMAND_TYPE_H_

#include <stdint.h>

namespace device {

// The type of a command that can be sent either to or from a U2F authenticator,
// i.e., a request or a response.
//
// TODO(pkalinnikov): Merge BLE-specific commands to this type.
enum class U2fCommandType : uint8_t {
  // PING and MSG commands are used as requests to U2F authenticators and the
  // corresponding responses. U2F raw messages are packed as MSG commands,
  // whereas PING is only aimed at checking the device's availability and can
  // encapsulate arbitrary data.
  PING = 0x81,
  MSG = 0x83,
  // INIT and WINK are specific to U2F over USB, used only as requests.
  INIT = 0x86,
  WINK = 0x88,
  // ERROR command is used as response only.
  ERROR = 0xbf,
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_COMMAND_TYPE_H_
