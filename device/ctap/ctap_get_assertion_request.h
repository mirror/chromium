// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_GET_ASSERTION_REQUEST_H_
#define DEVICE_CTAP_CTAP_GET_ASSERTION_REQUEST_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"
#include "device/ctap/ctap_request.h"
#include "device/ctap/public_key_credential_descriptor.h"

namespace device {

// Object that encapsulates request parameters for AuthenticatorGetAssertion as
// specified in the CTAP spec.
class CTAPGetAssertionRequest : public CTAPRequest {
 public:
  class CTAPGetAssertionRequestBuilder {
   public:
    CTAPGetAssertionRequestBuilder();
    ~CTAPGetAssertionRequestBuilder();

    void set_rp_id(std::string rp_id);
    void set_client_data_hash(std::vector<uint8_t> client_data_hash);
    void set_allow_list(std::vector<PublicKeyCredentialDescriptor> allow_list);
    void set_pin_auth(std::vector<uint8_t> pin_auth);
    void set_pin_protocol(uint8_t pin_protocol);

    base::Optional<CTAPGetAssertionRequest> BuildRequest();

   private:
    bool CheckRequiredParameters();

    base::Optional<std::string> rp_id_param_;
    base::Optional<std::vector<uint8_t>> client_data_hash_param_;
    base::Optional<std::vector<PublicKeyCredentialDescriptor>>
        allow_list_param_;
    base::Optional<std::vector<uint8_t>> pin_auth_param_;
    base::Optional<uint8_t> pin_protocol_param_;
  };

  CTAPGetAssertionRequest(
      std::string rp_id,
      std::vector<uint8_t> client_data_hash,
      base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list,
      base::Optional<std::vector<uint8_t>> pin_auth,
      base::Optional<uint8_t> pin_protocol);
  ~CTAPGetAssertionRequest() override;
  CTAPGetAssertionRequest(CTAPGetAssertionRequest&& that);

  base::Optional<std::vector<uint8_t>> SerializeToCBOR() const override;

 private:
  std::string rp_id_;
  std::vector<uint8_t> client_data_hash_;
  base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;

  DISALLOW_COPY_AND_ASSIGN(CTAPGetAssertionRequest);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_GET_ASSERTION_REQUEST_H_
