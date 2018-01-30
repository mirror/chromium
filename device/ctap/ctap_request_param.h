// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_REQUEST_PARAM_H_
#define DEVICE_CTAP_CTAP_REQUEST_PARAM_H_

#include <stdint.h>
#include <vector>

#include "base/optional.h"

namespace device {

class CTAPRequestParam {
 public:
  CTAPRequestParam();
  virtual ~CTAPRequestParam();
  virtual base::Optional<std::vector<uint8_t>> Encode() const = 0;
  virtual bool CheckU2fInteropCriteria() const;
  virtual std::vector<uint8_t> GetU2FApplicationParameter() const;
  virtual std::vector<uint8_t> GetU2FChallengeParameter() const;

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  virtual std::vector<std::vector<uint8_t>> GetU2FRegisteredKeysParameter()
      const;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_REQUEST_PARAM_H_
