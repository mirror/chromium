// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/ssl_platform_key_mac.h"

#include <CoreFoundation/CoreFoundation.h>

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/memory/ref_counted.h"
#include "net/ssl/ssl_private_key.h"
#include "net/ssl/ssl_private_key_test_util.h"
#include "net/test/cert_test_util.h"
#include "net/test/keychain_test_util_mac.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
namespace net {

namespace {

struct TestKey {
  const char* name;
  const char* cert_file;
  const char* key_file;
};

const TestKey kTestKeys[] = {
    {"RSA", "client_1.pem", "client_1.pk8"},
    {"ECDSA_P256", "client_4.pem", "client_4.pk8"},
    {"ECDSA_P384", "client_5.pem", "client_5.pk8"},
    {"ECDSA_P521", "client_6.pem", "client_6.pk8"},
};

std::string TestKeyToString(const testing::TestParamInfo<TestKey>& params) {
  return params.param.name;
}

}  // namespace

class SSLPlatformKeyMacTest : public testing::TestWithParam<TestKey> {};

TEST_P(SSLPlatformKeyMacTest, KeyMatches) {
  const TestKey& test_key = GetParam();

  // Load test data.
  scoped_refptr<X509Certificate> cert =
      ImportCertFromFile(GetTestCertsDirectory(), test_key.cert_file);
  ASSERT_TRUE(cert);

  std::string pkcs8;
  base::FilePath pkcs8_path =
      GetTestCertsDirectory().AppendASCII(test_key.key_file);
  ASSERT_TRUE(base::ReadFileToString(pkcs8_path, &pkcs8));

  // Create a temporary keychain.
  ScopedTestKeychain scoped_keychain;
  ASSERT_TRUE(scoped_keychain.Initialize());
  SecKeychainRef keychain = scoped_keychain.keychain();

  // Import cert and key to the keychain.
  base::ScopedCFTypeRef<SecIdentityRef> sec_identity(
      ImportCertAndKeyToKeychain(cert.get(), pkcs8, keychain));
  ASSERT_TRUE(sec_identity);

  // Finally, test the code to look up the key.
  scoped_refptr<SSLPrivateKey> key =
      CreateSSLPrivateKeyForSecIdentity(cert.get(), sec_identity.get());
  ASSERT_TRUE(key);

  // All Mac keys are expected to have the same preferences.
  std::vector<uint16_t> expected = {
      SSL_SIGN_RSA_PKCS1_SHA512, SSL_SIGN_ECDSA_SECP521R1_SHA512,
      SSL_SIGN_RSA_PKCS1_SHA384, SSL_SIGN_ECDSA_SECP384R1_SHA384,
      SSL_SIGN_RSA_PKCS1_SHA256, SSL_SIGN_ECDSA_SECP256R1_SHA256,
      SSL_SIGN_RSA_PKCS1_SHA1,   SSL_SIGN_ECDSA_SHA1,
  };
  EXPECT_EQ(expected, key->GetPreferences());

  TestSSLPrivateKeyMatches(key.get(), pkcs8);
}

INSTANTIATE_TEST_CASE_P(,
                        SSLPlatformKeyMacTest,
                        testing::ValuesIn(kTestKeys),
                        TestKeyToString);

}  // namespace net
