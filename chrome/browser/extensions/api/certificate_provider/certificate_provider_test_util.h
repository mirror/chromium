// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_CERTIFICATE_PROVIDER_CERTIFICATE_PROVIDER_TEST_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_API_CERTIFICATE_PROVIDER_CERTIFICATE_PROVIDER_TEST_UTIL_H_

#include <string>
#include <vector>

namespace crypto {
class RSAPrivateKey;
}

namespace chromeos {
namespace platform_keys_test_util {

// Signs |digest| with |key|, using the hash algorithm identified by
// |hash_algorithm_name|. This must be one of the supported hash algorithms as
// passed to the certificateProvider API's onSignDigestRequested event.
// If the hash algorithm was valid and signing succeeded, fills *|signature| and
// returns true. Otherwise, returns false.
bool RsaSign(const std::vector<uint8_t>& digest,
             const std::string& hash_algorithm_name,
             crypto::RSAPrivateKey* key,
             std::vector<uint8_t>* signature);

// Create a string that if evaluated in JavaScript returns a Uint8Array with
// |bytes| as content.
std::string JsUint8Array(const std::vector<uint8_t>& bytes);

}  // namespace platform_keys_test_util
}  // namespace chromeos

#endif  // CHROME_BROWSER_EXTENSIONS_API_CERTIFICATE_PROVIDER_CERTIFICATE_PROVIDER_TEST_UTIL_H_
