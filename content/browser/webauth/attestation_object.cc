// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/attestation_object.h"

#include <utility>

#include "content/browser/webauth/attestation_statement.h"
#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {

namespace {
constexpr char kAuthDataKey[] = "authData";
constexpr char kFormatKey[] = "fmt";
constexpr char kAttestationKey[] = "attStmt";
}  // namespace

AttestationObject::AttestationObject(
    std::unique_ptr<AuthenticatorData> data,
    std::unique_ptr<AttestationStatement> statement)
    : authenticator_data_(std::move(data)),
      attestation_statement_(std::move(statement)) {}

std::vector<uint8_t> AttestationObject::SerializeToCborEncodedBytes() {
  CBORValue::MapValue map;
  map[kFormatKey] = CBORValue(attestation_statement_->format_name());
  map[kAuthDataKey] = CBORValue(authenticator_data_->SerializeToByteArray());
  map[kAttestationKey] = CBORValue(attestation_statement_->GetAsCborMap());
  return CBORWriter::Write(CBORValue(map));
}

AttestationObject::~AttestationObject() {}

}  // namespace content