// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/attestation_data.h"

namespace content {
namespace authenticator_utils {

AttestationData::AttestationData(const std::vector<uint8_t>& aaguid,
                                 const std::vector<uint8_t>& length,
                                 const std::vector<uint8_t>& credential_id,
                                 const std::unique_ptr<PublicKey>& public_key)
    : aaguid_(aaguid),
      id_length_(length),
      credential_id_(credential_id),
      public_key_(public_key) {}

std::vector<uint8_t> AttestationData::SerializeToByteArray() {
  std::vector<uint8_t> attestation_data;
  std::vector<uint8_t> cbor_encoded_key =
      public_key_->GetCborEncodedByteArray();
  attestation_data.insert(attestation_data.end(), &aaguid_[0], &aaguid_[16]);
  attestation_data.insert(attestation_data.end(), &id_length_[0],
                          &id_length_[2]);
  attestation_data.insert(attestation_data.end(), credential_id_.begin(),
                          credential_id_.end());
  attestation_data.insert(attestation_data.end(), cbor_encoded_key.begin(),
                          cbor_encoded_key.end());
  return attestation_data;
}

AttestationData::~AttestationData() {}

}  // namespace authenticator_utils
}  // namespace content