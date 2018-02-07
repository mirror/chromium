// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_CTAP_MAKE_CREDENTIAL_REQUEST_PARAM_H_
#define DEVICE_FIDO_CTAP_MAKE_CREDENTIAL_REQUEST_PARAM_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/optional.h"
#include "device/fido/public_key_credential_descriptor.h"
#include "device/fido/public_key_credential_params.h"
#include "device/fido/public_key_credential_rp_entity.h"
#include "device/fido/public_key_credential_user_entity.h"
#include "device/fido/u2f_transferable_param.h"

namespace device {

// Object containing request parameters for AuthenticatorMakeCredential command
// as specified in
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html
class CtapMakeCredentialRequestParam : public U2fTransferableParam {
 public:
  CtapMakeCredentialRequestParam(
      std::vector<uint8_t> client_data_hash,
      PublicKeyCredentialRPEntity rp,
      PublicKeyCredentialUserEntity user,
      PublicKeyCredentialParams public_key_credential_params);
  CtapMakeCredentialRequestParam(CtapMakeCredentialRequestParam&& that);
  CtapMakeCredentialRequestParam& operator=(
      CtapMakeCredentialRequestParam&& that);
  ~CtapMakeCredentialRequestParam() override;

  base::Optional<std::vector<uint8_t>> Encode() const override;
  bool CheckU2fInteropCriteria() const override;
  std::vector<uint8_t> GetU2fApplicationParameter() const override;
  std::vector<std::vector<uint8_t>> GetU2fRegisteredKeysParameter()
      const override;
  std::vector<uint8_t> GetU2fChallengeParameter() const override;

  CtapMakeCredentialRequestParam& SetUserVerificationRequired(
      bool user_verfication_required);
  CtapMakeCredentialRequestParam& SetResidentKey(bool resident_key);
  CtapMakeCredentialRequestParam& SetExcludeList(
      std::vector<PublicKeyCredentialDescriptor> exclude_list);
  CtapMakeCredentialRequestParam& SetPinAuth(std::vector<uint8_t> pin_auth);
  CtapMakeCredentialRequestParam& SetPinProtocol(uint8_t pin_protocol);

 private:
  std::vector<uint8_t> client_data_hash_;
  PublicKeyCredentialRPEntity rp_;
  PublicKeyCredentialUserEntity user_;
  PublicKeyCredentialParams public_key_credentials_;
  bool user_verification_required_ = false;
  bool resident_key_ = false;

  base::Optional<std::vector<PublicKeyCredentialDescriptor>> exclude_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;

  DISALLOW_COPY_AND_ASSIGN(CtapMakeCredentialRequestParam);
};

}  // namespace device

#endif  // DEVICE_FIDO_CTAP_MAKE_CREDENTIAL_REQUEST_PARAM_H_
