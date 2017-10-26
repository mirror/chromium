// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/collected_client_data.h"

#include "base/base64url.h"
#include "base/json/json_writer.h"

namespace content {

// static
std::unique_ptr<CollectedClientData> CollectedClientData::Create(
    std::string relying_party_id,
    std::vector<uint8_t> challenge) {
  //  The base64url encoding of options.challenge.
  std::string encoded_challenge;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(challenge.data()),
                        challenge.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded_challenge);

  // TokenBinding is optional, and missing if the browser doesn't support it.
  // It is present and set to the constant "unused" if the browser
  // supports Token Binding, but is not using it to talk to the origin.
  // TODO(kpaulhamus): Fetch and add the Token Binding ID public key used to
  // communicate with the origin.
  return std::make_unique<CollectedClientData>(std::move(encoded_challenge),
                                               std::move(relying_party_id),
                                               "SHA-256", "unused");
}

CollectedClientData::CollectedClientData(std::string challenge,
                                         std::string origin,
                                         std::string hash_alg,
                                         std::string token_binding_id)
    : challenge_(std::move(challenge)),
      origin_(std::move(origin)),
      hash_alg_(std::move(hash_alg)),
      token_binding_id_(std::move(token_binding_id)) {}

std::string CollectedClientData::SerializeToJson() {
  base::DictionaryValue client_data;
  client_data.SetString(authenticator_internal::kChallengeKey, challenge_);

  // The serialization of callerOrigin.
  client_data.SetString(authenticator_internal::kOriginKey, origin_);

  // The recognized algorithm name of the hash algorithm selected by the client
  // for generating the hash of the serialized client data.
  client_data.SetString(authenticator_internal::kHashAlg, hash_alg_);

  // TokenBinding is optional, and missing if the browser doesn't support it.
  // It is present and set to the constant "unused" if the browser
  // supports Token Binding, but is not using it to talk to the origin.
  // TODO(kpaulhamus): Fetch and add the Token Binding ID public key used to
  // communicate with the origin.
  client_data.SetString(authenticator_internal::kTokenBindingKey,
                        token_binding_id_);

  std::string json;
  base::JSONWriter::Write(client_data, &json);
  return json;
}

CollectedClientData::~CollectedClientData() {}

}  // namespace content