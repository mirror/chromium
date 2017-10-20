// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/attestation_object.h"

#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {

AttestationObject::AttestationObject(
    std::unique_ptr<AuthenticatorData> data,
    std::unique_ptr<AttestationStatement> statement)
    : authenticator_data_(std::move(data)),
      attestation_statement_(std::move(statement)) {}

std::vector<uint8_t> AttestationObject::SerializeToCborEncodedByteArray() {
  std::vector<uint8_t> attestation_object;
  base::flat_map<std::string, CBORValue> map;
  map[kFmtKey] = CBORValue(attestation_statement_->GetFormat());
  map[kAuthKey] = CBORValue(authenticator_data_->SerializeToByteArray());
  map[kAttestationKey] = CBORValue(attestation_statement_->GetAsCborMap());
  std::vector<uint8_t> cbor = CBORWriter::Write(CBORValue(map));
  attestation_object.insert(attestation_object.end(), cbor.begin(), cbor.end());
  return attestation_object;
}

AttestationObject::~AttestationObject() {}

}  // namespace content