// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_TRANSFERABLE_PARAM_H_
#define DEVICE_FIDO_U2F_TRANSFERABLE_PARAM_H_

#include <stdint.h>

#include <vector>

#include "base/optional.h"
#include "device/fido/fido_request_param.h"

namespace device {
// Virtual interface implemented by CTAPMakeCredentialRequestParam and
// CTAPGetAssertionRequestParam classes as they are parameters for requests
// that have direct correspondence with U2F register and U2F sign requests.
class U2fTransferableParam : public virtual FidoRequestParam {
 public:
  U2fTransferableParam();
  ~U2fTransferableParam() override;

  base::Optional<std::vector<uint8_t>> Encode() const override = 0;
  virtual bool CheckU2fInteropCriteria() const = 0;
  virtual std::vector<uint8_t> GetU2fApplicationParameter() const = 0;
  virtual std::vector<uint8_t> GetU2fChallengeParameter() const = 0;

  // The application parameter is the SHA-256 hash of the UTF-8 encoding of
  // the application identity (i.e. relying_party_id) of the application
  // requesting the registration.
  virtual std::vector<std::vector<uint8_t>> GetU2fRegisteredKeysParameter()
      const = 0;
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_TRANSFERABLE_PARAM_H_
