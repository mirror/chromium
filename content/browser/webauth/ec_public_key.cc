// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/ec_public_key.h"

#include <utility>

#include "content/browser/webauth/authenticator_utils.h"
#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {

// static
std::unique_ptr<ECPublicKey> ECPublicKey::Create(
    std::string algorithm,
    std::vector<uint8_t> u2f_data) {
  // Extract the key, which is located after the first byte of the response
  // (which is a reserved byte).
  // The uncompressed form consists of 65 bytes:
  // - a constant 0x04 prefix
  // - the 32-byte x coordinate
  // - the 32-byte y coordinate.
  int start = 2;  // Account for reserved byte and 0x04 prefix.
  std::vector<uint8_t> x = authenticator_utils::Splice(u2f_data, start, 32);
  std::vector<uint8_t> y =
      authenticator_utils::Splice(u2f_data, start + 32, 32);
  return std::make_unique<ECPublicKey>(std::move(algorithm), x, y);
}

ECPublicKey::ECPublicKey(std::string algorithm,
                         std::vector<uint8_t> x,
                         std::vector<uint8_t> y)
    : PublicKey(std::move(algorithm)),
      x_coordinate_(std::move(x)),
      y_coordinate_(std::move(y)) {
  CHECK_EQ(x_coordinate_.size(), 32u);
  CHECK_EQ(y_coordinate_.size(), 32u);
}

std::vector<uint8_t> ECPublicKey::SerializeToCborEncodedBytes() {
  CBORValue::MapValue map;
  map["alg"] = CBORValue(algorithm_);
  map["x"] = CBORValue(x_coordinate_);
  map["y"] = CBORValue(y_coordinate_);
  std::vector<uint8_t> public_key = CBORWriter::Write(CBORValue(map));
  return public_key;
}

ECPublicKey::~ECPublicKey() {}

}  // namespace content