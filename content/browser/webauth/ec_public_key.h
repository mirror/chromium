// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_EC_PUBLIC_KEY_H_
#define CONTENT_BROWSER_WEBAUTH_EC_PUBLIC_KEY_H_

#include "content/browser/webauth/public_key.h"
#include "content/common/content_export.h"

namespace content {

namespace {
constexpr char kEs256[] = "ES256";
}  // namespace

// An uncompressed ECPublicKey consisting of 64 bytes:
// - the 32-byte x coordinate
// - the 32-byte y coordinate.
class CONTENT_EXPORT ECPublicKey : public PublicKey {
 public:
  static std::unique_ptr<ECPublicKey> Create(
      const std::string algorithm,
      const std::vector<uint8_t>& u2f_data);

  // Produces a CBOR-encoded public key encoded in the following format:
  // { alg: eccAlgName, x: biguint, y: biguint }
  // where eccAlgName = "ES256" / "ES384" / "ES512"
  std::vector<uint8_t> SerializeToCborEncodedByteArray() override;
  ~ECPublicKey() override;

 private:
  ECPublicKey(const std::string algorithm,
              const std::vector<uint8_t> x,
              const std::vector<uint8_t> y);

  const std::vector<uint8_t> x_coordinate_;
  const std::vector<uint8_t> y_coordinate_;

  DISALLOW_COPY_AND_ASSIGN(ECPublicKey);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_EC_PUBLIC_KEY_H_
