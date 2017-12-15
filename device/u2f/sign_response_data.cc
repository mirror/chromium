// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/sign_response_data.h"

#include "base/base64url.h"
#include "device/u2f/attested_credential_data.h"
#include "device/u2f/ec_public_key.h"
#include "device/u2f/fido_attestation_statement.h"

namespace device {

// static
std::unique_ptr<SignResponseData> SignResponseData::CreateFromU2fSignResponse(
    std::string relying_party_id,
    const std::vector<uint8_t>& u2f_data,
    const std::vector<uint8_t>& key_handle) {
  if (key_handle.empty()) {
    return nullptr;
  }

  // Extract the 4-byte counter following the flag byte.
  if (u2f_data.size() < 5u) {
    return nullptr;
  }
  std::vector<uint8_t> counter(&u2f_data[1], &u2f_data[5]);

  // Construct the authenticator data.
  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(
          relying_party_id, u2f_data[0], counter,
          std::unique_ptr<AttestedCredentialData>() /* no attestation */);

  // Extract the signature from the remainder of the U2fResponse bytes.
  std::vector<uint8_t> signature(u2f_data.begin() + 5, u2f_data.end());
  if (signature.empty()) {
    return nullptr;
  }
  return std::make_unique<SignResponseData>(std::move(key_handle),
                                            std::move(authenticator_data),
                                            std::move(signature));
}

SignResponseData::SignResponseData(
    std::vector<uint8_t> credential_id,
    std::unique_ptr<AuthenticatorData> authenticator_data,
    std::vector<uint8_t> signature)
    : ResponseData(std::move(credential_id)),
      authenticator_data_(std::move(authenticator_data)),
      signature_(std::move(signature)) {}

std::vector<uint8_t> SignResponseData::GetAuthenticatorDataBytes() {
  return authenticator_data_->SerializeToByteArray();
}

SignResponseData::~SignResponseData() {}

}  // namespace device
