// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/certificate_provider/certificate_provider_test_util.h"

#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "crypto/rsa_private_key.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"

namespace chromeos {

namespace platform_keys_test_util {

namespace {
// Defines the mapping of hash algorithm names, as passed to the
// certificateProvider API's onSignDigestRequested event, to RSA NIDs.
const struct NameRsaNidPair {
  const char* const name;
  const int rsa_nid;
} kHashAlgorithmNames[] = {
    {"SHA1", NID_sha1},
    {"SHA256", NID_sha256},
    {"SHA384", NID_sha384},
    {"SHA512", NID_sha512},
};

// Parses a hash algorithm name, as passed to the certificaterProvider API's
// onSignDigestRequested event, into a RSA NID identifying the hash algorithm.
// If the hash algorithm name was recognized, sets *|rsa_hash_nid| and returns
// true. Otherwise, returns false.
bool ParseHashAlgorithmToRSAHashNID(const std::string hash_algorithm_name,
                                    int* rsa_hash_nid) {
  for (const NameRsaNidPair& hash_candidate_pair : kHashAlgorithmNames) {
    if (hash_algorithm_name == hash_candidate_pair.name) {
      *rsa_hash_nid = hash_candidate_pair.rsa_nid;
      return true;
    }
  }
  return false;
}

}  // namespace

bool RsaSign(const std::vector<uint8_t>& digest,
             const std::string& hash_algorithm,
             crypto::RSAPrivateKey* key,
             std::vector<uint8_t>* signature) {
  // See net::SSLPrivateKey::SignDigest for the expected padding and DigestInfo
  // prefixing.
  RSA* rsa_key = EVP_PKEY_get0_RSA(key->key());
  if (!rsa_key)
    return false;

  int rsa_hash_nid = NID_sha1;
  if (!ParseHashAlgorithmToRSAHashNID(hash_algorithm, &rsa_hash_nid))
    return false;

  uint8_t* prefixed_digest = nullptr;
  size_t prefixed_digest_len = 0;
  int is_alloced = 0;
  if (!RSA_add_pkcs1_prefix(&prefixed_digest, &prefixed_digest_len, &is_alloced,
                            rsa_hash_nid, digest.data(), digest.size())) {
    return false;
  }
  size_t len = 0;
  signature->resize(RSA_size(rsa_key));
  const int rv =
      RSA_sign_raw(rsa_key, &len, signature->data(), signature->size(),
                   prefixed_digest, prefixed_digest_len, RSA_PKCS1_PADDING);
  if (is_alloced)
    free(prefixed_digest);

  if (rv) {
    signature->resize(len);
    return true;
  }
  signature->clear();
  return false;
}

std::string JsUint8Array(const std::vector<uint8_t>& bytes) {
  std::string res = "new Uint8Array([";
  for (const uint8_t byte : bytes) {
    res += base::UintToString(byte);
    res += ", ";
  }
  res += "])";
  return res;
}

}  // namespace platform_keys_test_util

}  // namespace chromeos
