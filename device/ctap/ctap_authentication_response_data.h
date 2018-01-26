// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_AUTHENTICATION_RESPONSE_DATA_H_
#define DEVICE_CTAP_CTAP_AUTHENTICATION_RESPONSE_DATA_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

namespace device {

// Base class for RegisterResponseData and SignResponseData.
class CTAPAuthenticationResponseData {
 public:
  std::string GetId() const;
  const std::vector<uint8_t>& raw_id() const { return raw_id_; }

 protected:
  explicit CTAPAuthenticationResponseData(std::vector<uint8_t> credential_id);

  CTAPAuthenticationResponseData();

  // Moveable.
  CTAPAuthenticationResponseData(CTAPAuthenticationResponseData&& other);
  CTAPAuthenticationResponseData& operator=(
      CTAPAuthenticationResponseData&& other);

  ~CTAPAuthenticationResponseData();

  std::vector<uint8_t> raw_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CTAPAuthenticationResponseData);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_AUTHENTICATION_RESPONSE_DATA_H_
