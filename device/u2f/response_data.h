// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_RESPONSE_DATA_H_
#define DEVICE_U2F_RESPONSE_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

namespace device {

// Base class for RegisterResponseData and SignResponseData.
class ResponseData {
 public:
  virtual ~ResponseData();

  std::string GetId();
  const std::vector<uint8_t>& raw_id() { return raw_id_; }

 protected:
  ResponseData(std::vector<uint8_t> credential_id);
  const std::vector<uint8_t> raw_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResponseData);
};

}  // namespace device

#endif  // DEVICE_U2F_RESPONSE_DATA_H_
