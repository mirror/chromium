// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_PUBLIC_KEY_H_
#define CONTENT_BROWSER_WEBAUTH_PUBLIC_KEY_H_

#include <vector>

#include "base/values.h"

namespace content {
namespace authenticator_utils {

// A credential public key that is used in AttestationData.
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data.
class PublicKey {
 public:
  PublicKey(const std::string algorithm);

  // The credential public key as a CBOR map defined by the following CDDL:
  // pubKey = $pubKeyFmt
  // ; All public key formats must include an alg name
  // pubKeyTemplate = { alg: text }
  // pubKeyTemplate .within $pubKeyFmt
  //
  // pubKeyFmt /= eccPubKey
  // eccPubKey = { alg: eccAlgName, x: biguint, y: biguint }
  // eccAlgName = "ES256" / "ES384" / "ES512"
  virtual std::vector<uint8_t> GetCborEncodedByteArray();
  virtual ~PublicKey();

 protected:
  std::string algorithm_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PublicKey);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_PUBLIC_KEY_H_
