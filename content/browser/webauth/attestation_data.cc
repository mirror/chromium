// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/attestation_data.h"

#include "base/logging.h"
#include "base/numerics/safe_math.h"

namespace content {

// static
std::unique_ptr<AttestationData> AttestationData::CreateFromU2fResponse(
    const std::vector<uint8_t>& u2f_data,
    std::vector<uint8_t> aaguid,
    std::unique_ptr<PublicKey> public_key) {
  // Extract the length length of the credential (i.e. of U2FResponse key
  // handle). Length is big endian and is located at position 66 in the data.
  // Note that U2F responses only use one byte for length.
  CHECK_GE(u2f_data.size(), authenticator_internal::kU2fResponseLengthPos);
  std::vector<uint8_t> id_length(2, 0);
  id_length[1] = u2f_data[authenticator_internal::kU2fResponseLengthPos];
  uint32_t id_end_byte = authenticator_internal::kU2fResponseIdStartPos +
                         base::checked_cast<uint32_t>(id_length[1]);
  CHECK_GE(u2f_data.size(), id_end_byte);

  // Extract the credential id (i.e. key handle).
  std::vector<uint8_t> credential_id;
  credential_id.insert(
      credential_id.end(),
      &u2f_data[authenticator_internal::kU2fResponseIdStartPos],
      &u2f_data[id_end_byte]);

  return std::make_unique<AttestationData>(
      std::move(aaguid), std::move(id_length), std::move(credential_id),
      std::move(public_key));
}

AttestationData::AttestationData(std::vector<uint8_t> aaguid,
                                 std::vector<uint8_t> length,
                                 std::vector<uint8_t> credential_id,
                                 std::unique_ptr<PublicKey> public_key)
    : aaguid_(std::move(aaguid)),
      id_length_(std::move(length)),
      credential_id_(std::move(credential_id)),
      public_key_(std::move(public_key)) {
  CHECK_EQ(aaguid_.size(), 16u);
  CHECK_EQ(id_length_.size(), 2u);
}

const std::vector<uint8_t>& AttestationData::GetCredentialId() {
  return credential_id_;
}

std::vector<uint8_t> AttestationData::SerializeToByteArray() {
  std::vector<uint8_t> attestation_data;
  std::vector<uint8_t> cbor_encoded_key =
      public_key_->SerializeToCborEncodedByteArray();
  attestation_data.insert(attestation_data.end(), aaguid_.begin(),
                          aaguid_.end());
  attestation_data.insert(attestation_data.end(), id_length_.begin(),
                          id_length_.end());
  attestation_data.insert(attestation_data.end(), credential_id_.begin(),
                          credential_id_.end());
  attestation_data.insert(attestation_data.end(), cbor_encoded_key.begin(),
                          cbor_encoded_key.end());
  return attestation_data;
}

AttestationData::~AttestationData() {}

}  // namespace content