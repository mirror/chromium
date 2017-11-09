// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/register_response_data.h"

#include <utility>

#include "content/browser/webauth/authenticator_utils.h"
#include "content/browser/webauth/ec_public_key.h"
#include "content/browser/webauth/fido_attestation_statement.h"

namespace content {

// static
std::unique_ptr<RegisterResponseData> RegisterResponseData::Create(
    std::unique_ptr<CollectedClientData> client_data,
    std::vector<uint8_t> u2f_data) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::Create(authenticator_utils::kEs256, u2f_data);

  // Construct the attestation data.
  // AAGUID is zeroed out for U2F responses.
  std::vector<uint8_t> aaguid(16, 0);
  std::unique_ptr<AttestationData> attestation_data =
      AttestationData::CreateFromU2fResponse(u2f_data, aaguid,
                                             std::move(public_key));

  // Extract the credential_id for packing into the reponse data.
  std::vector<uint8_t> credential_id = attestation_data->credential_id();

  // Construct the authenticator data.
  // The counter is zeroed out for Register requests.
  std::vector<uint8_t> counter(4, 0);
  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(client_data->SerializeToJson(), 0x00, counter,
                                std::move(attestation_data));

  // Set flags on authenticator data.
  authenticator_data->set_test_of_user_presence_flag();
  authenticator_data->set_attestation_flag();

  // Construct the attestation statement.
  std::unique_ptr<FidoAttestationStatement> fido_attestation_statement =
      FidoAttestationStatement::Create(u2f_data);

  // Construct the attestation object.
  std::unique_ptr<AttestationObject> attestation_object(new AttestationObject(
      std::move(authenticator_data), std::move(fido_attestation_statement)));

  return std::make_unique<RegisterResponseData>(std::move(client_data),
                                                std::move(credential_id),
                                                std::move(attestation_object));
}

RegisterResponseData::RegisterResponseData(
    std::unique_ptr<CollectedClientData> client_data,
    std::vector<uint8_t> credential_id,
    std::unique_ptr<AttestationObject> object)
    : client_data_(std::move(client_data)),
      raw_id_(std::move(credential_id)),
      attestation_object_(std::move(object)) {}

RegisterResponseData::~RegisterResponseData() {}

}  // namespace content