// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_DATA_H_
#define DEVICE_CTAP_CTAP_DATA_H_

#include <stdint.h>
#include <vector>

#include "device/ctap/ctap_response_code.h"

namespace device {

class CTAPData {
 public:
  CTAPData();
  virtual ~CTAPData();

 protected:
  union {
    uint8_t cmd_;
    uint8_t response_;
  };
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_DATA_H_
