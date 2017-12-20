// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_MAKE_CREDENTIAL_H_
#define DEVICE_CTAP_CTAP_MAKE_CREDENTIAL_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"
#include "device/ctap/ctap_request.h"
#include "device/ctap/public_key_credential_descriptor.h"
#include "device/ctap/public_key_credential_params.h"
#include "device/ctap/public_key_credential_rp_entity.h"
#include "device/ctap/public_key_credential_user_entity.h"

namespace device {

// Object containing request parameters for AuthenticatorMakeCredential command
// as specified in https://fidoalliance.org/specs/fido-v2.0-rd-20170927/
// fido-client-to-authenticator-protocol-v2.0-rd-20170927.html.
//
// Null optional will be returned if required parameters for the request object
// are not provided.
class CTAPMakeCredentialRequest : public CTAPRequest {
 public:
  class MakeCredentialRequestBuilder {
   public:
    MakeCredentialRequestBuilder();
    ~MakeCredentialRequestBuilder();

    void set_client_data_hash(std::vector<uint8_t> client_data_hash);
    void set_rp(PublicKeyCredentialRPEntity rp);
    void set_user(PublicKeyCredentialUserEntity user);
    void set_public_key_credential_params(
        PublicKeyCredentialParams public_key_credential_params);
    void set_exclude_list(
        std::vector<PublicKeyCredentialDescriptor> exclude_list);
    void set_pin_auth(std::vector<uint8_t> pin_auth);
    void set_pin_protocol(uint8_t pin_protocol);

    base::Optional<CTAPMakeCredentialRequest> BuildRequest();

   private:
    bool CheckRequiredParameters();

    base::Optional<std::vector<uint8_t>> client_data_hash_param_;
    base::Optional<PublicKeyCredentialRPEntity> rp_param_;
    base::Optional<PublicKeyCredentialUserEntity> user_param_;
    base::Optional<PublicKeyCredentialParams> public_key_credentials_params_;
    base::Optional<std::vector<PublicKeyCredentialDescriptor>>
        exclude_list_param_;
    base::Optional<std::vector<uint8_t>> pin_auth_param_;
    base::Optional<uint8_t> pin_protocol_param_;
  };

  ~CTAPMakeCredentialRequest() override;
  CTAPMakeCredentialRequest(CTAPMakeCredentialRequest&& that);

  base::Optional<std::vector<uint8_t>> SerializeToCBOR() const override;

 private:
  CTAPMakeCredentialRequest(
      std::vector<uint8_t> client_data_hash,
      PublicKeyCredentialRPEntity rp,
      PublicKeyCredentialUserEntity user,
      PublicKeyCredentialParams public_key_credential_params,
      base::Optional<std::vector<PublicKeyCredentialDescriptor>> exclude_list,
      base::Optional<std::vector<uint8_t>> pin_auth,
      base::Optional<uint8_t> pin_protocol);

  std::vector<uint8_t> client_data_hash_;
  PublicKeyCredentialRPEntity rp_;
  PublicKeyCredentialUserEntity user_;
  PublicKeyCredentialParams public_key_credentials_;
  base::Optional<std::vector<PublicKeyCredentialDescriptor>> exclude_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;

  DISALLOW_COPY_AND_ASSIGN(CTAPMakeCredentialRequest);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_MAKE_CREDENTIAL_H_
