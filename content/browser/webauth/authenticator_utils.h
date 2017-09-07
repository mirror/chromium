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

// Creates PublicKeyCredentialInfo from authenticator response and clientData.
webauth::mojom::PublicKeyCredentialInfoPtr CONTENT_EXPORT
ConstructPublicKeyCredentialInfo(const std::string& clientDataJSON,
                                 const std::vector<uint8_t>& data);

// Construct a response to a navigator.credentials.create request from
// the data returned from a U2F authenticator.
// Respones contains a CBOR attestation_object per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation
webauth::mojom::AuthenticatorResponsePtr CONTENT_EXPORT
ConstructAuthenticatorAttestationResponse(const std::string& relying_party_id,
                                          const std::vector<uint8_t>& data,
                                          std::vector<uint8_t>& credential_id);

// Extract an attestation data and attestation statement from the U2FRegister
// response.
// Returns the credential ID to be set as the PublicKeyCredential raw_id.
// |attestation_data| includes AAGUID, credential length, credential ID,
// and the public key
// (per https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data).
// |attestation_statement| is a CBOR object that can be in a number of
// formats (https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation).
void CONTENT_EXPORT
ParseU2fRegisterResponse(const std::vector<uint8_t>& u2f_data,
                         std::vector<uint8_t>& attestation_data,
                         std::vector<uint8_t>& attestation_statement,
                         std::vector<uint8_t>& credential_id);

// Pack the 65 bytes of the public key from U2FResponse as a CBOR object, per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data.
void CONTENT_EXPORT
ConstructPublicKeyObject(const std::vector<uint8_t>& u2f_data,
                         std::vector<uint8_t>& public_key);

// Construct the attestation_statement, defined by the CDDL at
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation
void CONTENT_EXPORT
ConstructFidoU2fAttestationStatement(const std::vector<uint8_t>& u2f_data,
                                     std::vector<uint8_t>& cbor,
                                     int begin);

// Serialize the final attestation object.
void CONTENT_EXPORT
SerializeAttestationObject(std::string attestation_format,
                           std::vector<uint8_t> authenticator_data,
                           std::vector<uint8_t> attestation_statement,
                           std::vector<uint8_t>& attestation_object);

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_