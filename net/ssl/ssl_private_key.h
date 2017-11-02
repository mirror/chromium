// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SSL_SSL_PRIVATE_KEY_H_
#define NET_SSL_SSL_PRIVATE_KEY_H_

#include <stdint.h>

#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "net/base/net_errors.h"

namespace net {

// An interface for a private key for use with SSL client authentication. A
// private key may be used with multiple signature algorithms, so methods use
// |SSL_SIGN_*| constants from BoringSSL, which correspond to TLS
// SignatureScheme values.
//
// Note that although ECDSA constants are named like
// |SSL_SIGN_ECDSA_SECP256R1_SHA256|, they may be used with any curve for
// purposes of this API. This descrepancy is due to differences between TLS 1.2
// and TLS 1.3.
class SSLPrivateKey : public base::RefCountedThreadSafe<SSLPrivateKey> {
 public:
  using SignCallback = base::Callback<void(Error, const std::vector<uint8_t>&)>;

  SSLPrivateKey() {}

  // Returns the algorithms that are supported by the key in decreasing
  // preference for TLS 1.2 and later. Note that |SSL_SIGN_RSA_PKCS1_MD5_SHA1|
  // is only used by TLS 1.1 and earlier and should not be in this list.
  //
  // The caller will discard any algorithms that are not applicable for the
  // particular key type (as parsed from the client certificate), so the list
  // for, e.g., a P-256 key may include |SSL_SIGN_RSA_PKCS1_SHA256|. This is a
  // convenience so SSLPrivateKey implementations may list the entire
  virtual std::vector<uint16_t> GetPreferences() = 0;

  // Asynchronously signs an |input| with the specified TLS signing
  // algorithm. |input| must already have been hashed by the corresponding hash
  // function (see |SSL_get_signature_algorithm_digest|). On completion, it
  // calls |callback| with the signature or an error code if the operation
  // failed.
  //
  // TODO(davidben): This API does not support algorithms without a
  // prehash, notably Ed25519. Replace this with a function that takes the
  // unhashed input.
  virtual void SignDigest(uint16_t algorithm,
                          const base::StringPiece& input,
                          const SignCallback& callback) = 0;

 protected:
  virtual ~SSLPrivateKey() {}

 private:
  friend class base::RefCountedThreadSafe<SSLPrivateKey>;
  DISALLOW_COPY_AND_ASSIGN(SSLPrivateKey);
};

}  // namespace net

#endif  // NET_SSL_SSL_PRIVATE_KEY_H_
