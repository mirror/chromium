// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_REQUEST_COMMAND_H_
#define DEVICE_CTAP_CTAP_REQUEST_COMMAND_H_

#include <stdint.h>

namespace device {

enum class CTAPRequestCommand : uint8_t {
  AUTHENTICATOR_MAKE_CREDENTIAL = 0x01,
  AUTHENTICATOR_GET_ASSERTION = 0x02,
  AUTHENTICATOR_GET_NEXT_ASSERTION = 0x08,
  AUTHENTICATOR_CANCEL = 0x03,
  AUTHENTICATOR_GET_INFO = 0x04,
  AUTHENTICATOR_CLIENT_PIN = 0x06,
  AUTHENTICATOR_RESET = 0x07,
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_REQUEST_COMMAND_H_
