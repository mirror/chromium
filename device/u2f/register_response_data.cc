// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/register_response_data.h"

#include <utility>

#include "device/u2f/attested_credential_data.h"
#include "device/u2f/ec_public_key.h"
#include "device/u2f/fido_attestation_statement.h"
#include "device/u2f/u2f_parsing_utils.h"

namespace device {

// static
std::unique_ptr<RegisterResponseData>
RegisterResponseData::CreateFromU2fRegisterResponse(
    std::string relying_party_id,
    std::vector<uint8_t> u2f_data) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::ExtractFromU2fRegistrationResponse(u2f_parsing_utils::kEs256,
                                                      u2f_data);

  // Construct the attestation data.
  // AAGUID is zeroed out for U2F responses.
  std::vector<uint8_t> aaguid(16u, 0u);
  std::unique_ptr<AttestedCredentialData> attested_data =
      AttestedCredentialData::CreateFromU2fRegisterResponse(
          u2f_data, std::move(aaguid), std::move(public_key));

  // Extract the credential_id for packing into the reponse data.
  std::vector<uint8_t> credential_id = attested_data->credential_id();

  // Construct the authenticator data.
  // The counter is zeroed out for Register requests.
  std::vector<uint8_t> counter(4u, 0u);
  AuthenticatorData::Flags flags =
      static_cast<AuthenticatorData::Flags>(
          AuthenticatorData::Flag::TEST_OF_USER_PRESENCE) |
      static_cast<AuthenticatorData::Flags>(
          AuthenticatorData::Flag::ATTESTATION);

  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(relying_party_id, flags, std::move(counter),
                                std::move(attested_data));

  // Construct the attestation statement.
  std::unique_ptr<FidoAttestationStatement> fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(u2f_data);

  // Construct the attestation object.
  auto attestation_object = std::make_unique<AttestationObject>(
      std::move(authenticator_data), std::move(fido_attestation_statement));

  return std::make_unique<RegisterResponseData>(std::move(credential_id),
                                                std::move(attestation_object));
}

RegisterResponseData::RegisterResponseData(
    std::vector<uint8_t> credential_id,
    std::unique_ptr<AttestationObject> object)
    : ResponseData(std::move(credential_id)),
      attestation_object_(std::move(object)) {}

std::vector<uint8_t> RegisterResponseData::GetCBOREncodedAttestationObject() {
  return attestation_object_->SerializeToCBOREncodedBytes();
}

RegisterResponseData::~RegisterResponseData() {}

}  // namespace device
