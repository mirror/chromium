// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_SIGN_RESPONSE_DATA_H_
#define DEVICE_U2F_SIGN_RESPONSE_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "device/u2f/authenticator_data.h"
#include "device/u2f/response_data.h"

namespace device {

// Unpacks a U2F sign response into a FIDO2 response.
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/ \
// fido-client-to-authenticator-protocol-v2.0-rd-20170927.html# \
// fig-u2f-compat-getAssertion
class SignResponseData : public ResponseData {
 public:
  static base::Optional<SignResponseData> CreateFromU2fSignResponse(
      std::string relying_party_id,
      std::vector<uint8_t> u2f_data,
      std::vector<uint8_t> key_handle);

  SignResponseData();

  SignResponseData(std::vector<uint8_t> credential_id,
                   std::unique_ptr<AuthenticatorData> authenticator_data,
                   std::vector<uint8_t> signature);

  SignResponseData(SignResponseData&& other);
  SignResponseData& operator=(SignResponseData&& other);

  ~SignResponseData() override;

  std::vector<uint8_t> GetAuthenticatorDataBytes() const;
  const std::vector<uint8_t>& signature() const { return signature_; }

 private:
  std::unique_ptr<AuthenticatorData> authenticator_data_;
  std::vector<uint8_t> signature_;
};

}  // namespace device

#endif  // DEVICE_U2F_SIGN_RESPONSE_DATA_H_
