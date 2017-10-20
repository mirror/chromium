// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/ec_public_key.h"

#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {

// static
std::unique_ptr<ECPublicKey> ECPublicKey::Create(
    const std::string algorithm,
    const std::vector<uint8_t>& u2f_data) {
  // Extract the key, which is located after the first byte of the response
  // (which is a reserved byte).
  // The uncompressed form consists of 65 bytes:
  // - a constant 0x04 prefix
  // - the 32-byte x coordinate
  // - the 32-byte y coordinate.
  int start = 2;  // Account for reserved byte and 0x04 prefix.
  std::vector<uint8_t> x(&u2f_data[start], &u2f_data[start + 32]);
  std::vector<uint8_t> y(&u2f_data[start + 32],
                         &u2f_data[kU2fResponseKeyLength + 1]);
  std::unique_ptr<ECPublicKey> key(new ECPublicKey(std::move(algorithm), x, y));
  return key;
}

ECPublicKey::ECPublicKey(const std::string algorithm,
                         const std::vector<uint8_t> x,
                         const std::vector<uint8_t> y)
    : PublicKey(std::move(algorithm)),
      x_coordinate_(std::move(x)),
      y_coordinate_(std::move(y)) {
  CHECK_EQ(32u, x.size());
  CHECK_EQ(32u, y.size());
}

std::vector<uint8_t> ECPublicKey::SerializeToCborEncodedByteArray() {
  base::flat_map<std::string, CBORValue> map;
  map["alg"] = CBORValue(algorithm_);
  map["x"] = CBORValue(x_coordinate_);
  map["y"] = CBORValue(y_coordinate_);
  std::vector<uint8_t> public_key = CBORWriter::Write(CBORValue(map));
  return public_key;
}

ECPublicKey::~ECPublicKey() {}

}  // namespace content