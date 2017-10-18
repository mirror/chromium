// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_data.h"

#include "crypto/sha2.h"

namespace content {
namespace authenticator_utils {

AuthenticatorData::AuthenticatorData(
    const std::string& relying_party_id,
    const uint8_t flags,
    const std::vector<uint8_t>& counter,
    const std::unique_ptr<AttestationData>& data)
    : relying_party_id_(relying_party_id),
      flags_(flags),
      counter_(counter),
      attestation_data_(std::move(data)) {}

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
  authenticator_data.insert(authenticator_data.end(), &counter_[0],
                            &counter_[4]);
  std::vector<uint8_t> attestation_bytes =
      attestation_data_->SerializeToByteArray();
  authenticator_data.insert(authenticator_data.end(), attestation_bytes.begin(),
                            attestation_bytes.end());
  return authenticator_data;
}

AuthenticatorData::~AuthenticatorData() {}

}  // namespace authenticator_utils
}  // namespace content