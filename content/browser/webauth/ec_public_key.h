// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_EC_PUBLIC_KEY_H_
#define CONTENT_BROWSER_WEBAUTH_EC_PUBLIC_KEY_H_

#include "content/browser/webauth/public_key.h"

namespace content {
namespace authenticator_utils {

// An uncompressed ECPublicKey consisting of 64 bytes:
// - the 32-byte x coordinate
// - the 32-byte y coordinate.
class ECPublicKey : public PublicKey {
 public:
  ECPublicKey(const std::string algorithm,
              const std::vector<uint8_t>& x,
              const std::vector<uint8_t>& y);

  std::vector<uint8_t> GetCborEncodedByteArray() override;
  ~ECPublicKey() override;

 private:
  std::vector<uint8_t> x_coordinate_;
  std::vector<uint8_t> y_coordinate_;

  DISALLOW_COPY_AND_ASSIGN(ECPublicKey);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_EC_PUBLIC_KEY_H_
