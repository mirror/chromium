// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_utils.h"

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/webauth/cbor/cbor_writer.h"
#include "crypto/sha2.h"

namespace content {

namespace authenticator_utils {

// CBOR key values
constexpr char kAuthKey[] = "authData";
constexpr char kFmtKey[] = "fmt";
constexpr char kU2fFormat[] = "fido-u2f";
constexpr char kAttestationKey[] = "attStmt";

// Flag values
const unsigned char kTestOfUserPresenceFlag = 1 << 0;
const unsigned char kAttestationFlag = 1 << 6;

// U2FResponse byte positions
const uint32_t kU2fResponseLengthPos = 66;
const uint32_t kU2fResponseIdStartPos = 67;
const uint32_t kU2fResponseKeyLength = 65;

// Serializes the |value| to a JSON string and returns the result.
std::string SerializeValueToJSON(const base::Value& value) {
  std::string json;
  base::JSONWriter::Write(value, &json);
  return json;
}

// Token Binding ID: optional; missing if the browser doesn't support it.
// TODO (https://crbug/757502): Add extensions support.
std::string BuildClientData(const std::string& relying_party_id,
                            const std::vector<uint8_t>& challenge) {
  base::DictionaryValue client_data;
  //  The base64url encoding of options.challenge.
  std::string encoded_challenge;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(challenge.data()),
                        challenge.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded_challenge);
  client_data.SetString(kChallengeKey, encoded_challenge);

  // The serialization of callerOrigin.
  client_data.SetString(kOriginKey, relying_party_id);

  // The recognized algorithm name of the hash algorithm selected by the client
  // for generating the hash of the serialized client data.
  client_data.SetString(kHashAlg, "SHA-256");

  // TokenBinding is optional, and missing if the browser doesn't support it.
  // It is present and set to the constant "unused" if the browser
  // supports Token Binding, but is not using it to talk to the origin.
  // TODO(kpaulhamus): Fetch and add the Token Binding ID public key used to
  // communicate with the origin.
  client_data.SetString(kTokenBindingKey, "unused");
  return SerializeValueToJSON(client_data);
}

webauth::mojom::PublicKeyCredentialInfoPtr ConstructPublicKeyCredentialInfo(
    const std::string& client_data_json,
    const std::vector<uint8_t>& data) {
  webauth::mojom::PublicKeyCredentialInfoPtr credentialInfo =
      webauth::mojom::PublicKeyCredentialInfo::New();
  std::vector<uint8_t> credential_id;

  credentialInfo->client_data_json =
      std::vector<uint8_t>(client_data_json.begin(), client_data_json.end());

  // Extract replying_party_id to pack into an AuthenticatorAttestationResponse.
  std::unique_ptr<base::Value> client_data =
      base::JSONReader::Read(std::string(client_data_json));
  base::DictionaryValue* client_data_dictionary;
  client_data->GetAsDictionary(&client_data_dictionary);

  std::string relying_party_id;
  client_data_dictionary->GetString(kOriginKey, &relying_party_id);

  credentialInfo->response = ConstructAuthenticatorAttestationResponse(
      relying_party_id, data, credential_id);

  // Set the identifiers.
  credentialInfo->raw_id = credential_id;
  std::string id;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(credential_id.data()),
                        credential_id.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  credentialInfo->id = id;

  return credentialInfo;
}

// TODO maybe keep this in authenticator_impl?
webauth::mojom::AuthenticatorResponsePtr
ConstructAuthenticatorAttestationResponse(const std::string& relying_party_id,
                                          const std::vector<uint8_t>& data,
                                          std::vector<uint8_t>& credential_id) {
  std::vector<uint8_t> authenticator_data;
  std::vector<uint8_t> attestation_data;
  std::vector<uint8_t> attestation_statement;
  webauth::mojom::AuthenticatorResponsePtr response =
      webauth::mojom::AuthenticatorResponse::New();

  // First, construct an authenticator data structure per
  // https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-authenticator-data.
  // |rpidhash|flags|counter|attestData|extensions.
  std::vector<uint8_t> rp_id_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, &rp_id_hash.front(),
                           rp_id_hash.size());
  authenticator_data.insert(authenticator_data.end(), rp_id_hash.begin(),
                            rp_id_hash.end());

  // Set flags.
  uint8_t flags = 0x00;
  flags |= kTestOfUserPresenceFlag;
  flags |= kAttestationFlag;
  authenticator_data.insert(authenticator_data.end(), flags);

  // Signature counter, zero for registration responses.
  uint8_t counter[4] = {0};
  authenticator_data.insert(authenticator_data.end(), &counter[0], &counter[4]);

  // Only U2F responses are currently supported.
  ParseU2fRegisterResponse(data, attestation_data, attestation_statement,
                           credential_id);

  // Add the attestation data to authenticator_data.
  authenticator_data.insert(authenticator_data.end(), attestation_data.begin(),
                            attestation_data.end());

  // Build the CBOR attestation object using the U2F attestation format.
  SerializeAttestationObject(kU2fFormat, authenticator_data,
                             attestation_statement,
                             response->attestation_object);
  return response;
}

void ParseU2fRegisterResponse(const std::vector<uint8_t>& u2f_data,
                              std::vector<uint8_t>& attestation_data,
                              std::vector<uint8_t>& attestation_statement,
                              std::vector<uint8_t>& credential_id) {
  // First, construct WebAuthN attestation data.
  // This consists of the AAGUID (16 bytes), Length (2 bytes),
  // Credential ID (L bytes), and Credential Public Key (65 bytes)

  // AAGUID is zeroed out for U2F responses.
  std::vector<uint8_t> aaguid(16, 0);
  attestation_data.insert(attestation_data.end(), &aaguid[0], &aaguid[16]);

  // Length of the credential (i.e. len of U2FResponse key handle). Big Endian.
  // Located at postion 66 (U2F responses only use one byte for length).
  std::vector<uint8_t> id_length(2, 0);

  id_length[1] = u2f_data[kU2fResponseLengthPos];

  // attestation_data.insert(attestation_data.end(), id_length, id_length + 2);
  attestation_data.insert(attestation_data.end(), &id_length[0], &id_length[2]);

  uint32_t id_end_byte = kU2fResponseIdStartPos +
                         ((base::checked_cast<uint32_t>(id_length[0]) << 8) |
                          (base::checked_cast<uint32_t>(id_length[1])));

  // Set Credential ID in attestation_data with the key handle bytes.
  attestation_data.insert(attestation_data.end(),
                          &u2f_data[kU2fResponseIdStartPos],
                          &u2f_data[id_end_byte]);

  // Set the Credential ID to use with raw_id, id.
  credential_id.insert(credential_id.end(), &u2f_data[kU2fResponseIdStartPos],
                       &u2f_data[id_end_byte]);

  // Add the CBOR public key object to the end of attestation_data.
  std::vector<uint8_t> public_key;
  ConstructPublicKeyObject(u2f_data, public_key);
  attestation_data.insert(attestation_data.end(), public_key.begin(),
                          public_key.end());

  // Then, build a U2F attestation statement with the remainder of the data.
  // Currently, only the "fido-u2f" format is supported.
  ConstructFidoU2fAttestationStatement(u2f_data, attestation_statement,
                                       id_end_byte);
  return;
}

void ConstructPublicKeyObject(const std::vector<uint8_t>& u2f_data,
                              std::vector<uint8_t>& public_key) {
  // The key is located after the first byte of the response, which is reserved.
  // The uncompressed form consists of 65 bytes:
  // - a constant 0x04 prefix
  // - the 32-byte x coordinate
  // - the 32-byte y coordinate.
  int start = 2;  // Account for reserved byte and 0x04 prefix.
  std::vector<uint8_t> x_coordinate(&u2f_data[start], &u2f_data[start + 32]);
  std::vector<uint8_t> y_coordinate(&u2f_data[start + 32],
                                    &u2f_data[kU2fResponseKeyLength + 1]);

  MapValue map;
  map.Insert("alg", "ES256");
  map.Insert("x", x_coordinate);
  map.Insert("y", y_coordinate);
  CBORWriter::Write(map, &public_key);
  return;
}

void ConstructFidoU2fAttestationStatement(
    const std::vector<uint8_t>& u2f_data,
    std::vector<uint8_t>& attestation_statement,
    int begin) {
  std::vector<uint8_t> x5c;
  std::vector<uint8_t> signature;

  // Parse x509 cert to get cert length (which is variable).
  int num_bytes = 0;
  uint32_t cert_length = 0;
  // The first x509 byte is a tag, so we skip it.
  uint32_t first_length_byte =
      base::checked_cast<uint32_t>(u2f_data[begin + 1]);

  // If the first length byte is less than 127, it is the length. If it is
  // greater than 128, it indicates the number of following bytes that encode
  // the length.
  if (first_length_byte > 127) {
    num_bytes = first_length_byte - 128;

    // x509 cert length, interpreted big-endian.
    for (int i = 1; i <= num_bytes; i++) {
      // Account for tag byte and length details byte.
      cert_length |= base::checked_cast<uint32_t>(u2f_data[begin + 1 + i]
                                                  << ((num_bytes - i) * 8));
    }
  } else {
    cert_length = first_length_byte;
  }

  int end = begin + 1 /* tag byte */ + 1 /* first length byte */
            + num_bytes /* # bytes of length */ + cert_length;

  x5c.insert(x5c.end(), &u2f_data[begin], &u2f_data[end]);
  // Remaining bytes are the signature.
  signature.insert(signature.end(), &u2f_data[end], &u2f_data[u2f_data.size()]);

  MapValue map;
  map.Insert("sig", signature);
  std::vector<CBORValue> array;
  array.push_back(CBORValue(x5c));
  map.Insert("x5c", array);
  CBORWriter::Write(map, &attestation_statement);
}

void SerializeAttestationObject(std::string attestation_format,
                                std::vector<uint8_t> authenticator_data,
                                std::vector<uint8_t> attestation_statement,
                                std::vector<uint8_t>& attestation_object) {
  MapValue map;
  map.Insert(kFmtKey, attestation_format);
  map.Insert(kAuthKey, authenticator_data);
  map.Insert(kAttestationKey, attestation_statement);
  CBORWriter::Write(map, &attestation_object);
}

}  // namespace authenticator_utils
}  // namespace content