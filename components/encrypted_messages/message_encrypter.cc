// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/encrypted_messages/message_encrypter.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_piece.h"
#include "components/encrypted_messages/encrypted_message.pb.h"
#include "crypto/aead.h"
#include "crypto/hkdf.h"
#include "crypto/random.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace encrypted_messages {

bool GetHkdfSubkeySecret(size_t subkey_length,
                         const uint8_t* private_key,
                         const uint8_t* public_key,
                         const char hkdf_label[],
                         std::string* secret) {
  uint8_t shared_secret[X25519_SHARED_KEY_LEN];
  if (!X25519(shared_secret, private_key, public_key))
    return false;

  // By mistake, the HKDF label here ends up with an extra null byte on
  // the end, due to using sizeof(kHkdfLabel) in the StringPiece
  // constructor instead of strlen(kHkdfLabel). Ideally this code should
  // be just passing kHkdfLabel directly into the HKDF constructor.
  //
  // TODO(carlosil): Using strlen + 1 while the server code is changed,
  // it was migrated to strlen since hkdf_label is now an argument (so
  // once the server side is changed, the only change left is to remove
  // the +1 here).
  // TODO(estark): fix this in coordination with the server-side code --
  // perhaps by rolling the public key version forward and using the
  // version to decide whether to use the extra-null-byte version of the
  // label. https://crbug.com/517746
  crypto::HKDF hkdf(base::StringPiece(reinterpret_cast<char*>(shared_secret),
                                      sizeof(shared_secret)),
                    "" /* salt */,
                    base::StringPiece(hkdf_label, strlen(hkdf_label) + 1),
                    0 /* key bytes */, 0 /* iv bytes */, subkey_length);

  *secret = hkdf.subkey_secret().as_string();
  return true;
}

bool EncryptSerializedMessage(const uint8_t* server_public_key,
                              uint32_t server_public_key_version,
                              const char hkdf_label[],
                              const std::string& message,
                              EncryptedMessage* encrypted_message) {
  // Generate an ephemeral key pair to generate a shared secret.
  uint8_t public_key[X25519_PUBLIC_VALUE_LEN];
  uint8_t private_key[X25519_PRIVATE_KEY_LEN];

  crypto::RandBytes(private_key, sizeof(private_key));
  X25519_public_from_private(public_key, private_key);

  crypto::Aead aead(crypto::Aead::AES_128_CTR_HMAC_SHA256);
  std::string key;
  if (!GetHkdfSubkeySecret(aead.KeyLength(), private_key,
                           reinterpret_cast<const uint8_t*>(server_public_key),
                           hkdf_label, &key)) {
    LOG(ERROR) << "Error getting subkey secret.";
    return false;
  }
  aead.Init(&key);

  // Use an all-zero nonce because the key is random per-message.
  std::string nonce(aead.NonceLength(), '\0');

  std::string ciphertext;
  if (!aead.Seal(message, nonce, std::string(), &ciphertext)) {
    LOG(ERROR) << "Error sealing message.";
    return false;
  }

  encrypted_message->set_encrypted_message(ciphertext);
  encrypted_message->set_server_public_key_version(server_public_key_version);
  encrypted_message->set_client_public_key(reinterpret_cast<char*>(public_key),
                                           sizeof(public_key));
  encrypted_message->set_algorithm(
      EncryptedMessage::AEAD_ECDH_AES_128_CTR_HMAC_SHA256);
  return true;
}

// Used only by tests.
bool DecryptMessage(const uint8_t server_private_key[32],
                    const char hkdf_label[],
                    const EncryptedMessage& encrypted_message,
                    std::string* decrypted_serialized_message) {
  crypto::Aead aead(crypto::Aead::AES_128_CTR_HMAC_SHA256);
  std::string key;
  if (!GetHkdfSubkeySecret(aead.KeyLength(), server_private_key,
                           reinterpret_cast<const uint8_t*>(
                               encrypted_message.client_public_key().data()),
                           hkdf_label, &key)) {
    LOG(ERROR) << "Error getting subkey secret.";
    return false;
  }
  aead.Init(&key);

  // Use an all-zero nonce because the key is random per-message.
  std::string nonce(aead.NonceLength(), 0);

  return aead.Open(encrypted_message.encrypted_message(), nonce, std::string(),
                   decrypted_serialized_message);
}

}  // namespace encrypted_messages
