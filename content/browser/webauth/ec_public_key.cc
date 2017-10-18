// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/ec_public_key.h"

#include "content/browser/webauth/cbor/cbor_writer.h"

namespace content {
namespace authenticator_utils {

ECPublicKey::ECPublicKey(const std::string algorithm,
                         const std::vector<uint8_t>& x,
                         const std::vector<uint8_t>& y)
    : PublicKey(algorithm), x_coordinate_(x), y_coordinate_(y) {}

std::vector<uint8_t> ECPublicKey::GetCborEncodedByteArray() {
  base::flat_map<std::string, CBORValue> map;
  map["alg"] = CBORValue(algorithm_);
  map["x"] = CBORValue(x_coordinate_);
  map["y"] = CBORValue(y_coordinate_);
  std::vector<uint8_t> public_key = CBORWriter::Write(CBORValue(map));
  return public_key;
}

ECPublicKey::~ECPublicKey() {}

}  // namespace authenticator_utils
}  // namespace content