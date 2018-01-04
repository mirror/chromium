// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_RESPONSE_H_
#define DEVICE_CTAP_CTAP_RESPONSE_H_

#include <stdint.h>
#include <vector>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"
#include "device/ctap/ctap_constants.h"

namespace device {

class CTAPResponse {
 public:
  explicit CTAPResponse(constants::CTAPDeviceResponseCode response_code);
  virtual ~CTAPResponse();
  constants::CTAPDeviceResponseCode response_code() const {
    return response_code_;
  }

 protected:
  constants::CTAPDeviceResponseCode response_code_;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_RESPONSE_H_
