// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/fido_attestation_statement.h"

#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {
namespace authenticator_utils {

constexpr char kFormat[] = "fido-u2f";
constexpr char kSigKey[] = "sig";
constexpr char kX5cKey[] = "x5c";

FidoAttestationStatement::FidoAttestationStatement(
    const std::vector<uint8_t>& signature,
    const std::vector<std::vector<uint8_t>>& x5c)
    : AttestationStatement(kFormat), signature_(signature), x509_certs_(x5c) {}

base::flat_map<std::string, CBORValue>
FidoAttestationStatement::GetAsCborData() {
  base::flat_map<std::string, CBORValue> attstmt_map;
  attstmt_map[kSigKey] = CBORValue(signature_);

  std::vector<CBORValue> array;
  for (auto cert : x509_certs_) {
    array.push_back(CBORValue(cert));
  }
  attstmt_map[kX5cKey] = CBORValue(array);
  return attstmt_map;
}

FidoAttestationStatement::~FidoAttestationStatement() {}

}  // namespace authenticator_utils
}  // namespace content