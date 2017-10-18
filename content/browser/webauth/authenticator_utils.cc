// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_utils.h"

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "content/browser/webauth/cbor/cbor_writer.h"
#include "content/browser/webauth/ec_public_key.h"
#include "content/browser/webauth/fido_attestation_statement.h"
#include "crypto/sha2.h"

namespace content {

namespace authenticator_utils {

// U2FResponse byte positions
const uint32_t kU2fResponseLengthPos = 66;
const uint32_t kU2fResponseIdStartPos = 67;
const uint32_t kU2fResponseKeyLength = 65;

constexpr char kEs256[] = "ES256";

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

  // TODO (https://crbug/757502): Add extensions support.
  std::string json;
  base::JSONWriter::Write(client_data, &json);
  return json;
}

// TODO include the diagram from the CTAP PR.
std::unique_ptr<RegisterResponseData> ParseU2fRegisterResponse(
    const std::string& client_data_json,
    const std::vector<uint8_t>& data) {
  // Extract the key, which is located after the first byte of the response
  // (which is a reserved byte).
  // The uncompressed form consists of 65 bytes:
  // - a constant 0x04 prefix
  // - the 32-byte x coordinate
  // - the 32-byte y coordinate.
  int start = 2;  // Account for reserved byte and 0x04 prefix.
  std::vector<uint8_t> x(&data[start], &data[start + 32]);
  std::vector<uint8_t> y(&data[start + 32], &data[kU2fResponseKeyLength + 1]);
  std::unique_ptr<ECPublicKey> public_key(new ECPublicKey(kEs256, x, y));

  // Extract the length length of the credential (i.e. of U2FResponse key
  // handle). Length is big endian and is located at position 66 in the data.
  // Note that U2F responses only use one byte for length.
  std::vector<uint8_t> id_length(2, 0);
  id_length[1] = data[kU2fResponseLengthPos];
  uint32_t id_end_byte = kU2fResponseIdStartPos +
                         ((base::checked_cast<uint32_t>(id_length[0]) << 8) |
                          (base::checked_cast<uint32_t>(id_length[1])));

  // Extract the credential id (i.e. key handle).
  std::vector<uint8_t> credential_id;
  credential_id.insert(credential_id.end(), &data[kU2fResponseIdStartPos],
                       &data[id_end_byte]);

  // Parse x509 cert to get cert length (which is variable).
  // TODO: Support responses with multiple certs.
  int num_bytes = 0;
  uint32_t cert_length = 0;
  // The first x509 byte is a tag, so we skip it.
  uint32_t first_length_byte =
      base::checked_cast<uint32_t>(data[id_end_byte + 1]);

  // If the first length byte is less than 127, it is the length. If it is
  // greater than 128, it indicates the number of following bytes that encode
  // the length.
  if (first_length_byte > 127) {
    num_bytes = first_length_byte - 128;

    // x509 cert length, interpreted big-endian.
    for (int i = 1; i <= num_bytes; i++) {
      // Account for tag byte and length details byte.
      cert_length |= base::checked_cast<uint32_t>(data[id_end_byte + 1 + i]
                                                  << ((num_bytes - i) * 8));
    }
  } else {
    cert_length = first_length_byte;
  }

  int end = id_end_byte + 1 /* tag byte */ + 1 /* first length byte */
            + num_bytes /* # bytes of length */ + cert_length;

  std::vector<std::vector<uint8_t>> x509_certs;
  std::vector<uint8_t> x5c;
  x5c.insert(x5c.end(), &data[id_end_byte], &data[end]);
  x509_certs.push_back(x5c);

  // The remaining bytes are the signature.
  std::vector<uint8_t> signature;
  signature.insert(signature.end(), &data[end], &data[data.size()]);

  // Construct the Attestation Data.
  // AAGUID is zeroed out for U2F responses.
  std::vector<uint8_t> aaguid(16, 0);
  std::unique_ptr<AttestationData> attestation_data(new AttestationData(
      aaguid, id_length, credential_id, std::move(public_key)));

  // Extract replying_party_id to pack into authenticator_data.
  std::string relying_party_id;
  base::DictionaryValue* client_data_dictionary;
  std::unique_ptr<base::Value> client_data =
      base::JSONReader::Read(std::string(client_data_json));
  client_data->GetAsDictionary(&client_data_dictionary);
  client_data_dictionary->GetString(kOriginKey, &relying_party_id);

  // The counter is zeroed out for Register requests.
  std::vector<uint8_t> counter(4, 0);
  std::unique_ptr<AuthenticatorData> authenticator_data(new AuthenticatorData(
      relying_party_id, 0x00, counter, std::move(attestation_data)));
  authenticator_data->SetTestOfUserPresenceFlag();
  authenticator_data->SetAttestationFlag();

  std::unique_ptr<FidoAttestationStatement> fido_attestation_statement(
      new FidoAttestationStatement(signature, x509_certs));
  std::unique_ptr<AttestationObject> attestation_object(new AttestationObject(
      std::move(authenticator_data), std::move(fido_attestation_statement)));

  std::unique_ptr<RegisterResponseData> response_data(new RegisterResponseData(
      client_data_json, credential_id, std::move(attestation_object)));
  return response_data;
}

}  // namespace authenticator_utils
}  // namespace content