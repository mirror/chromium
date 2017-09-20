// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "crypto/encryptor.h"
#include "crypto/openssl_util.h"
#include "crypto/random.h"
#include "crypto/rsa_private_key.h"
#include "crypto/symmetric_key.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"

using std::endl;
using std::cout;

#define PRINT(format, ...) printf(format "\n", ##__VA_ARGS__);
#define DEBUG(format, ...) printf(format "\n", ##__VA_ARGS__);
#define DEBUG(format, ...)

int RunTest() {
  std::string input;
  std::string output;
  std::string output_key;
  unsigned int INPUT_SIZE = 200 * 1024;
  unsigned int KEY_SIZE = 128;
  unsigned int RSA_KEY_SIZE = 2048;

  uint8_t* input_ptr =
      reinterpret_cast<uint8_t*>(base::WriteInto(&input, INPUT_SIZE + 1));
  crypto::RandBytes(input_ptr, INPUT_SIZE);

  std::unique_ptr<crypto::SymmetricKey> sym_key =
      crypto::SymmetricKey::GenerateRandomKey(
          crypto::SymmetricKey::Algorithm::AES, KEY_SIZE);

  std::unique_ptr<crypto::RSAPrivateKey> key =
      crypto::RSAPrivateKey::Create(RSA_KEY_SIZE);
  bssl::UniquePtr<RSA> rsa_key(EVP_PKEY_get1_RSA(key->key()));
  if (!rsa_key) {
    PRINT("Failed to create RSA key");
    return 0;
  }
  if (!RSA_check_key(rsa_key.get())) {
    PRINT("RSA key invalid");
    return 0;
  }
  unsigned int rsa_size = RSA_size(rsa_key.get());
  DEBUG("RSA_size: %d", rsa_size);

  crypto::Encryptor encryptor;
  if (!encryptor.Init(sym_key.get(), crypto::Encryptor::Mode::CBC,
                      "the iv: 16 bytes")) {
    PRINT("Encryptor failed to init.");
    return 0;
  }

  base::TimeTicks start = base::TimeTicks::Now();

  if (!encryptor.Encrypt(input, &output)) {
    PRINT("Encryptor failed to encrypt.");
    return 0;
  }

  uint8_t* output_key_ptr =
      reinterpret_cast<uint8_t*>(base::WriteInto(&output_key, rsa_size + 1));
  size_t len = 0;
  if (!RSA_encrypt(rsa_key.get(), &len, output_key_ptr,
                   rsa_size * 2 /* max size */,
                   reinterpret_cast<const uint8_t*>(sym_key->key().data()),
                   KEY_SIZE, RSA_PKCS1_PADDING)) {
    DEBUG("Failed to encrypt with RSA key.");
    return 0;
  }

  base::TimeDelta elapsed = base::TimeTicks::Now() - start;
  PRINT("Encrypted in %f ms", elapsed.InMillisecondsF());
  return 1;
}

int main() {
  PRINT("Hello, start crypto test.. ");

  for (int i = 0; i < 100; ++i) {
    RunTest();
  }
}
