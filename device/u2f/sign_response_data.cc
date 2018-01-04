// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/sign_response_data.h"

#include "base/base64url.h"
#include "base/optional.h"
#include "device/u2f/attested_credential_data.h"
#include "device/u2f/ec_public_key.h"
#include "device/u2f/fido_attestation_statement.h"

namespace device {

// static
base::Optional<SignResponseData> SignResponseData::CreateFromU2fSignResponse(
    std::string relying_party_id,
    std::vector<uint8_t> u2f_data,
    std::vector<uint8_t> key_handle) {
  if (key_handle.empty()) {
    return base::Optional<SignResponseData>();
  }

  // Extract the 4-byte counter following the flag byte.
  if (u2f_data.size() < 6u) {
    return base::Optional<SignResponseData>();
  }
  std::vector<uint8_t> counter(u2f_data.begin() + 1, u2f_data.begin() + 5);

  // Construct the authenticator data.
  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(relying_party_id, u2f_data[0], counter,
                                base::Optional<AttestedCredentialData>());

  // Extract the signature from the remainder of the U2fResponse bytes.
  std::vector<uint8_t> signature(u2f_data.begin() + 5, u2f_data.end());
  return base::Optional<SignResponseData>(
      SignResponseData(std::move(key_handle), std::move(authenticator_data),
                       std::move(signature)));
}

SignResponseData::SignResponseData() = default;

SignResponseData::SignResponseData(
    std::vector<uint8_t> credential_id,
    std::unique_ptr<AuthenticatorData> authenticator_data,
    std::vector<uint8_t> signature)
    : ResponseData(std::move(credential_id)),
      authenticator_data_(std::move(authenticator_data)),
      signature_(std::move(signature)) {}

SignResponseData::SignResponseData(SignResponseData&& other) = default;

SignResponseData& SignResponseData::operator=(SignResponseData&& other) =
    default;

SignResponseData::~SignResponseData() = default;

std::vector<uint8_t> SignResponseData::GetAuthenticatorDataBytes() const {
  return authenticator_data_->SerializeToByteArray();
}

}  // namespace device
