// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/sign_response_data.h"

#include "base/base64url.h"
#include "content/browser/webauth/attested_credential_data.h"
#include "content/browser/webauth/ec_public_key.h"
#include "content/browser/webauth/fido_attestation_statement.h"

namespace content {

// static
std::unique_ptr<SignResponseData> SignResponseData::CreateFromU2fSignResponse(
    std::unique_ptr<CollectedClientData> client_data,
    const std::vector<uint8_t>& u2f_data,
    const std::vector<uint8_t>& key_handle) {
  DCHECK(!key_handle.empty());

  // Extract the 4-byte counter following the flag byte.
  CHECK_GE(u2f_data.size(), 5u);
  std::vector<uint8_t> counter(&u2f_data[1], &u2f_data[5]);

  // Construct the authenticator data.
  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(
          client_data->SerializeToJson(), u2f_data[0], counter,
          std::unique_ptr<AttestedCredentialData>() /* no attestation */);

  // Extract the signature from the remainder of the U2fResponse bytes.
  std::vector<uint8_t> signature(u2f_data.begin() + 5, u2f_data.end());
  DCHECK(!signature.empty());
  return std::make_unique<SignResponseData>(
      std::move(client_data), std::move(key_handle),
      std::move(authenticator_data), std::move(signature));
}

SignResponseData::SignResponseData(
    std::unique_ptr<CollectedClientData> client_data,
    std::vector<uint8_t> credential_id,
    std::unique_ptr<AuthenticatorData> authenticator_data,
    std::vector<uint8_t> signature)
    : ResponseData(std::move(client_data), (std::move(credential_id))),
      authenticator_data_(std::move(authenticator_data)),
      signature_(std::move(signature)) {}

std::vector<uint8_t> SignResponseData::GetAuthenticatorDataBytes() {
  return authenticator_data_->SerializeToByteArray();
}

SignResponseData::~SignResponseData() {}

}  // namespace content
