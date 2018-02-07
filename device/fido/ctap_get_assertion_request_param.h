// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_CTAP_GET_ASSERTION_REQUEST_PARAM_H_
#define DEVICE_FIDO_CTAP_GET_ASSERTION_REQUEST_PARAM_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/optional.h"
#include "device/fido/public_key_credential_descriptor.h"
#include "device/fido/u2f_transferable_param.h"

namespace device {

// Object that encapsulates request parameters for AuthenticatorGetAssertion as
// specified in the CTAP spec.
class CtapGetAssertionRequestParam : public U2fTransferableParam {
 public:
  CtapGetAssertionRequestParam(std::string rp_id,
                               std::vector<uint8_t> client_data_hash);
  CtapGetAssertionRequestParam(CtapGetAssertionRequestParam&& that);
  CtapGetAssertionRequestParam& operator=(CtapGetAssertionRequestParam&& other);
  ~CtapGetAssertionRequestParam() override;

  base::Optional<std::vector<uint8_t>> Encode() const override;
  bool CheckU2fInteropCriteria() const override;
  std::vector<uint8_t> GetU2fApplicationParameter() const override;
  std::vector<uint8_t> GetU2fChallengeParameter() const override;
  std::vector<std::vector<uint8_t>> GetU2fRegisteredKeysParameter()
      const override;

  CtapGetAssertionRequestParam& SetUserVerificationRequired(
      bool user_verfication_required);
  CtapGetAssertionRequestParam& SetUserPresenceRequired(
      bool user_presence_required);
  CtapGetAssertionRequestParam& SetAllowList(
      std::vector<PublicKeyCredentialDescriptor> allow_list);
  CtapGetAssertionRequestParam& SetPinAuth(std::vector<uint8_t> pin_auth);
  CtapGetAssertionRequestParam& SetPinProtocol(uint8_t pin_protocol);

 private:
  std::string rp_id_;
  std::vector<uint8_t> client_data_hash_;
  bool user_verification_required_ = false;
  bool user_presence_required_ = true;

  base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;

  DISALLOW_COPY_AND_ASSIGN(CtapGetAssertionRequestParam);
};

}  // namespace device

#endif  // DEVICE_FIDO_CTAP_GET_ASSERTION_REQUEST_PARAM_H_
