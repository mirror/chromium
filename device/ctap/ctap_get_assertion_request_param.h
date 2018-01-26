// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_GET_ASSERTION_REQUEST_PARAM_H_
#define DEVICE_CTAP_CTAP_GET_ASSERTION_REQUEST_PARAM_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/optional.h"
#include "device/ctap/ctap_request_param.h"
#include "device/ctap/public_key_credential_descriptor.h"

namespace device {

// Object that encapsulates request parameters for AuthenticatorGetAssertion as
// specified in the CTAP spec.
class CTAPGetAssertionRequestParam : public CTAPRequestParam {
 public:
  CTAPGetAssertionRequestParam(std::string rp_id,
                               std::vector<uint8_t> client_data_hash);
  CTAPGetAssertionRequestParam(CTAPGetAssertionRequestParam&& that);
  CTAPGetAssertionRequestParam(const CTAPGetAssertionRequestParam& that);
  CTAPGetAssertionRequestParam& operator=(CTAPGetAssertionRequestParam&& other);
  CTAPGetAssertionRequestParam& operator=(
      const CTAPGetAssertionRequestParam& other);
  ~CTAPGetAssertionRequestParam() override;

  base::Optional<std::vector<uint8_t>> Encode() const override;
  bool CheckU2fInteropCriteria() const override;
  std::vector<uint8_t> GetU2FApplicationParameter() const override;
  std::vector<uint8_t> GetU2FChallengeParameter() const override;
  std::vector<std::vector<uint8_t>> GetU2FRegisteredKeysParameter()
      const override;

  CTAPGetAssertionRequestParam& SetUserVerificationRequired(
      bool user_verfication_required);
  CTAPGetAssertionRequestParam& SetUserPresenceRequired(
      bool user_presence_required);
  CTAPGetAssertionRequestParam& SetAllowList(
      std::vector<PublicKeyCredentialDescriptor> allow_list);
  CTAPGetAssertionRequestParam& SetPinAuth(std::vector<uint8_t> pin_auth);
  CTAPGetAssertionRequestParam& SetPinProtocol(uint8_t pin_protocol);

  const std::string& rp_id() const { return rp_id_; }
  const std::vector<uint8_t>& client_data_hash() { return client_data_hash_; }
  const base::Optional<std::vector<PublicKeyCredentialDescriptor>>& allow_list()
      const {
    return allow_list_;
  }

 private:
  std::string rp_id_;
  std::vector<uint8_t> client_data_hash_;
  bool user_verification_required_ = false;
  bool user_presence_required_ = true;

  base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_GET_ASSERTION_REQUEST_PARAM_H_
