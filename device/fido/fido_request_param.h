// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_REQUEST_PARAM_H_
#define DEVICE_FIDO_FIDO_REQUEST_PARAM_H_

#include <stdint.h>

#include <vector>

#include "base/optional.h"

namespace device {

class FidoRequestParam {
 public:
  FidoRequestParam();
  virtual ~FidoRequestParam();
  virtual base::Optional<std::vector<uint8_t>> Encode() const = 0;
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_REQUEST_PARAM_H_
