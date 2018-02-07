// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_CTAP_REQUEST_PARAM_H_
#define DEVICE_FIDO_CTAP_REQUEST_PARAM_H_

#include <stdint.h>
#include <vector>

#include "base/optional.h"

namespace device {

class CtapRequestParam {
 public:
  CtapRequestParam();
  virtual ~CtapRequestParam();
  virtual base::Optional<std::vector<uint8_t>> Encode() const = 0;

  // Below functions are only implemented by CTAPMakeCredentialRequestParam and
  // CTAPGetAssertionRequestParam classes as they are parameters for requests
  // that have direct correspondence with U2F register and U2F sign requests.
  virtual bool CheckU2fInteropCriteria() const;
  virtual std::vector<uint8_t> GetU2fApplicationParameter() const;
  virtual std::vector<uint8_t> GetU2fChallengeParameter() const;

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  virtual std::vector<std::vector<uint8_t>> GetU2fRegisteredKeysParameter()
      const;
};

}  // namespace device

#endif  // DEVICE_FIDO_CTAP_REQUEST_PARAM_H_
