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
  // PING command encapsulates arbitrary data. It is used both for requests, and
  // the corresponding responses containing the same data. The command is aimed
  // at checking the device's availability.
  CMD_PING = 0x81,
  // MSG command encapsulates a U2F raw message. It is used both for requests,
  // and the corresponding responses.
  CMD_MSG = 0x83,
  // INIT and WINK are specific to U2F over USB, used only as requests.
  CMD_INIT = 0x86,
  CMD_WINK = 0x88,
  // ERROR command is used as response only.
  CMD_ERROR = 0xBF,
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_COMMAND_TYPE_H_
