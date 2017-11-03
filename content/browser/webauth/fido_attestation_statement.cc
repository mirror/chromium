// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/fido_attestation_statement.h"

#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {

namespace {
constexpr char kFidoFormatName[] = "fido-u2f";
constexpr char kSignatureKey[] = "sig";
constexpr char kX509CertKey[] = "x5c";
}  // namespace

// static
std::unique_ptr<FidoAttestationStatement> FidoAttestationStatement::Create(
    const std::vector<uint8_t>& u2f_data) {
  std::vector<std::vector<uint8_t>> x509_certs;
  std::vector<uint8_t> x5c;

  CHECK_GE(u2f_data.size(), authenticator_internal::kU2fResponseLengthPos + 1);

  // Extract the length length of the credential (i.e. of U2FResponse key
  // handle). Length is big endian and is located at position 66 in the data.
  // Note that U2F responses only use one byte for length.
  std::vector<uint8_t> id_length(2, 0);
  id_length[1] = u2f_data[authenticator_internal::kU2fResponseLengthPos];
  uint32_t id_end_byte = authenticator_internal::kU2fResponseIdStartPos +
                         ((base::checked_cast<uint32_t>(id_length[0]) << 8) |
                          (base::checked_cast<uint32_t>(id_length[1])));

  CHECK_GE(u2f_data.size(), id_end_byte + 1);

  // Parse x509 cert to get cert length (which is variable).
  // TODO: Support responses with multiple certs.
  int num_bytes = 0;
  uint32_t cert_length = 0;
  // The first x509 byte is a tag, so we skip it.
  uint32_t first_length_byte =
      base::checked_cast<uint32_t>(u2f_data[id_end_byte + 1]);

  // If the first length byte is less than 127, it is the length. If it is
  // greater than 128, it indicates the number of following bytes that encode
  // the length.
  if (first_length_byte > 127) {
    num_bytes = first_length_byte - 128;

    // x509 cert length, interpreted big-endian.
    for (int i = 1; i <= num_bytes; i++) {
      // Account for tag byte and length details byte.
      cert_length |= base::checked_cast<uint32_t>(u2f_data[id_end_byte + 1 + i]
                                                  << ((num_bytes - i) * 8));
    }
  } else {
    cert_length = first_length_byte;
  }

  int end = id_end_byte + 1 /* tag byte */ + 1 /* first length byte */
            + num_bytes /* # bytes of length */ + cert_length;

  CHECK_GE(u2f_data.size(), base::checked_cast<uint64_t>(end));

  x5c.insert(x5c.end(), &u2f_data[id_end_byte], &u2f_data[end]);
  x509_certs.push_back(x5c);

  // The remaining bytes are the signature.
  std::vector<uint8_t> signature;
  signature.insert(signature.end(), &u2f_data[end], &u2f_data[u2f_data.size()]);
  return std::make_unique<FidoAttestationStatement>(signature, x509_certs);
}

FidoAttestationStatement::FidoAttestationStatement(
    std::vector<uint8_t> signature,
    std::vector<std::vector<uint8_t>> x5c)
    : AttestationStatement(kFidoFormatName),
      signature_(std::move(signature)),
      x509_certs_(std::move(x5c)) {}

CBORValue FidoAttestationStatement::GetAsCborMap() {
  CBORValue::MapValue attstmt_map;
  attstmt_map[kSignatureKey] = CBORValue(signature_);

  std::vector<CBORValue> array;
  for (auto cert : x509_certs_) {
    array.push_back(CBORValue(cert));
  }
  attstmt_map[kX509CertKey] = CBORValue(array);
  return CBORValue(attstmt_map);
}

FidoAttestationStatement::~FidoAttestationStatement() {}

}  // namespace content