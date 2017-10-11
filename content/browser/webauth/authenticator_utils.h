// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_

#include <vector>

#include "base/values.h"
#include "third_party/WebKit/public/platform/modules/webauth/authenticator.mojom.h"

namespace content {

namespace authenticator_utils {

// JSON key values
constexpr char kChallengeKey[] = "challenge";
constexpr char kOriginKey[] = "origin";
constexpr char kHashAlg[] = "hashAlg";
constexpr char kTokenBindingKey[] = "tokenBinding";

std::string SerializeValueToJSON(const base::Value& value);

// Build client_data per step 13 of
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#createCredential).
std::string CONTENT_EXPORT
BuildClientData(const std::string& relying_party_id,
                const std::vector<uint8_t>& challenge);

// Creates PublicKeyCredentialInfo from |data|, the response returned from a U2F
// authenticator, and with |clientDataJSON|.
webauth::mojom::PublicKeyCredentialInfoPtr CONTENT_EXPORT
ConstructPublicKeyCredentialInfo(const std::string& clientDataJSON,
                                 const std::vector<uint8_t>& data);

// Construct a response to a navigator.credentials.create request from
// |data|, the data returned from a U2F authenticator.
// Returns the |credential_id| to be set as the PublicKeyCredential raw_id.
// The response contains a CBOR attestation_object per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation
webauth::mojom::AuthenticatorResponsePtr CONTENT_EXPORT
ConstructAuthenticatorAttestationResponse(const std::string& relying_party_id,
                                          const std::vector<uint8_t>& data,
                                          std::vector<uint8_t>& credential_id);

// Parses |u2f_data| to construct |attestation_data| and extract
// attestation statement information. Returns the |credential_id| to be set as
// the PublicKeyCredential raw_id.
// |attestation_data| includes AAGUID, credential length, credential ID,
// and the public key
// (per https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data).
// |attestation_statement| is a CBOR object that can be in a number of
// formats (https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation).
void CONTENT_EXPORT
ParseU2fRegisterResponse(const std::vector<uint8_t>& u2f_data,
                         std::vector<uint8_t>& attestation_data,
                         std::vector<uint8_t>& signature,
                         std::vector<uint8_t>& x5c,
                         std::vector<uint8_t>& credential_id);

// Pack the 65 bytes of the public key from U2FResponse as a CBOR object, per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data.
void CONTENT_EXPORT
ConstructPublicKeyObject(const std::vector<uint8_t>& u2f_data,
                         std::vector<uint8_t>& public_key);

// Extracts and returns the data to be CBOR-encoded as a
// U2F attestation statement, per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation.
void CONTENT_EXPORT
ExtractU2fAttestationStatementData(const std::vector<uint8_t>& u2f_data,
                                   std::vector<uint8_t>& signature,
                                   std::vector<uint8_t>& x5c,
                                   int begin);

// Serialize the final attestation object with a u2f attestation statement
// defined by the CDDL at
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation.
void CONTENT_EXPORT
SerializeU2fAttestationObject(const std::vector<uint8_t>& authenticator_data,
                              const std::vector<uint8_t>& signature,
                              const std::vector<uint8_t>& x5c,
                              std::vector<uint8_t>& attestation_object);
}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_