// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_PUBLIC_KEY_H_
#define CONTENT_BROWSER_WEBAUTH_PUBLIC_KEY_H_

#include <vector>

#include "base/values.h"

namespace content {

namespace authenticator_internal {
const uint32_t kU2fResponseKeyLength = 65;
const uint32_t kU2fResponseLengthPos = 66;
const uint32_t kU2fResponseIdStartPos = 67;
}  // namespace authenticator_internal

// A credential public key that is used in AttestationData.
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data.
class PublicKey {
 public:
  // The credential public key as a CBOR map according to a format
  // defined per public key type.
  virtual std::vector<uint8_t> SerializeToCborEncodedByteArray() = 0;
  virtual ~PublicKey();

 protected:
  PublicKey(const std::string algorithm);
  const std::string algorithm_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PublicKey);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_PUBLIC_KEY_H_
