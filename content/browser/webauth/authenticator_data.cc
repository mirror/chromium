// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_data.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "content/browser/webauth/collected_client_data.h"
#include "crypto/sha2.h"

namespace content {

namespace {
// Flag values
constexpr uint8_t kTestOfUserPresenceFlag = 1 << 0;
constexpr uint8_t kAttestationFlag = 1 << 6;
}  // namespace

// static
std::unique_ptr<AuthenticatorData> AuthenticatorData::Create(
    std::string client_data_json,
    uint8_t flags,
    std::vector<uint8_t> counter,
    std::unique_ptr<AttestationData> data) {
  // Extract replying_party_id from client_data_json..
  std::string relying_party_id;
  base::DictionaryValue* client_data_dictionary;
  std::unique_ptr<base::Value> client_data =
      base::JSONReader::Read(client_data_json);
  client_data->GetAsDictionary(&client_data_dictionary);
  client_data_dictionary->GetString(authenticator_internal::kOriginKey,
                                    &relying_party_id);

  return std::make_unique<AuthenticatorData>(
      relying_party_id, flags, std::move(counter), std::move(data));
}

AuthenticatorData::AuthenticatorData(std::string relying_party_id,
                                     uint8_t flags,
                                     std::vector<uint8_t> counter,
                                     std::unique_ptr<AttestationData> data)
    : relying_party_id_(std::move(relying_party_id)),
      flags_(flags),
      counter_(std::move(counter)),
      attestation_data_(std::move(data)) {
  CHECK_EQ(counter_.size(), 4u);
}

void AuthenticatorData::SetTestOfUserPresenceFlag() {
  flags_ |= kTestOfUserPresenceFlag;
}

void AuthenticatorData::SetAttestationFlag() {
  flags_ |= kAttestationFlag;
}

std::vector<uint8_t> AuthenticatorData::SerializeToByteArray() {
  std::vector<uint8_t> authenticator_data;
  std::vector<uint8_t> rp_id_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id_, &rp_id_hash.front(),
                           rp_id_hash.size());
  authenticator_data.insert(authenticator_data.end(), rp_id_hash.begin(),
                            rp_id_hash.end());
  authenticator_data.insert(authenticator_data.end(), flags_);
  authenticator_data.insert(authenticator_data.end(), counter_.begin(),
                            counter_.end());
  std::vector<uint8_t> attestation_bytes =
      attestation_data_->SerializeToByteArray();
  authenticator_data.insert(authenticator_data.end(), attestation_bytes.begin(),
                            attestation_bytes.end());
  return authenticator_data;
}

AuthenticatorData::~AuthenticatorData() {}

}  // namespace content