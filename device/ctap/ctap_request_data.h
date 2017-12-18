// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_REQUEST_DATA_H_
#define DEVICE_CTAP_CTAP_REQUEST_DATA_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"
#include "device/ctap/ctap_data.h"
#include "device/ctap/public_key_credential_descriptor.h"
#include "device/ctap/public_key_credential_params.h"
#include "device/ctap/public_key_credential_rp_entity.h"
#include "device/ctap/public_key_credential_user_entity.h"

namespace device {

class CTAPMakeCredentialRequest : public CTAPObject {
 public:
  class MakeCredentialRequestBuilder {
   public:
    MakeCredentialRequestBuilder();
    ~MakeCredentialRequestBuilder();

    void SetClientDataHash(std::vector<uint8_t> client_data_hash);
    void SetRP(PublicKeyCredentialRPEntity rp);
    void SetUser(PublicKeyCredentialUserEntity user);
    void SetPublicKeyCredentialParams(
        PublicKeyCredentialParams public_key_credential_params);
    void SetExcludeList(
        std::vector<PublicKeyCredentialDescriptor> exclude_list);
    void SetPinAuth(std::vector<uint8_t> pin_auth);
    void SetPinProtocol(uint8_t pin_protocol);

    base::Optional<CTAPMakeCredentialRequest> BuildRequest();

   private:
    bool CheckRequiredParameters();

    base::Optional<std::vector<uint8_t>> client_data_hash_param_;
    base::Optional<PublicKeyCredentialRPEntity> rp_param_;
    base::Optional<PublicKeyCredentialUserEntity> user_param_;
    base::Optional<PublicKeyCredentialParams> public_key_credentials_param_;
    base::Optional<std::vector<PublicKeyCredentialDescriptor>>
        exclude_list_param_;
    base::Optional<std::vector<uint8_t>> pin_auth_param_;
    base::Optional<uint8_t> pin_protocol_param_;
  };

  ~CTAPMakeCredentialRequest() override;
  CTAPMakeCredentialRequest(CTAPMakeCredentialRequest&& that);

  const base::Optional<std::vector<uint8_t>> SerializeToCBOR() const;

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

class CTAPGetAssertionRequest : public CTAPObject {
 public:
  class CTAPGetAssertionRequestBuilder {
   public:
    CTAPGetAssertionRequestBuilder();
    ~CTAPGetAssertionRequestBuilder();

    void SetRPId(std::string rp_id);
    void SetClientDataHash(std::vector<uint8_t> client_data_hash);
    void SetAllowList(std::vector<PublicKeyCredentialDescriptor> allow_list);
    void SetPinAuth(std::vector<uint8_t> pin_auth);
    void SetPinProtocol(uint8_t pin_protocol);

    base::Optional<CTAPGetAssertionRequest> BuildRequest();

   private:
    base::Optional<std::string> rp_id_param_;
    base::Optional<std::vector<uint8_t>> client_data_hash_param_;
    base::Optional<std::vector<PublicKeyCredentialDescriptor>>
        allow_list_param_;
    base::Optional<std::vector<uint8_t>> pin_auth_param_;
    base::Optional<uint8_t> pin_protocol_param_;

    bool CheckRequiredParameters();
  };

  CTAPGetAssertionRequest(
      std::string rp_id,
      std::vector<uint8_t> client_data_hash,
      base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list_,
      base::Optional<std::vector<uint8_t>> pin_auth_,
      base::Optional<uint8_t> pin_protocol_);
  ~CTAPGetAssertionRequest() override;
  CTAPGetAssertionRequest(CTAPGetAssertionRequest&& that) noexcept;

  const base::Optional<std::vector<uint8_t>> SerializeToCBOR() const;

 private:
  std::string rp_id_;
  std::vector<uint8_t> client_data_hash_;
  base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;

  DISALLOW_COPY_AND_ASSIGN(CTAPGetAssertionRequest);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_REQUEST_DATA_H_
