// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ENCRYPTED_MESSAGES_MESSAGE_ENCRYPTER_H_
#define COMPONENTS_ENCRYPTED_MESSAGES_MESSAGE_ENCRYPTER_H_

#include <stdint.h>
#include <string>

namespace encrypted_messages {

class EncryptedMessage;

bool GetHkdfSubkeySecret(size_t subkey_length,
                         const uint8_t* private_key,
                         const uint8_t* public_key,
                         const char hkdf_label[],
                         std::string* secret);

bool EncryptSerializedMessage(const uint8_t* server_public_key,
                              uint32_t server_public_key_version,
                              const char hkdf_label[],
                              const std::string& message,
                              EncryptedMessage* encrypted_message);

bool DecryptMessage(const uint8_t server_private_key[32],
                    const char hkdf_label[],
                    const EncryptedMessage& encrypted_message,
                    std::string* decrypted_serialized_message);

}  // namespace encrypted_messages

#endif  // COMPONENTS_ENCRYPTED_MESSAGES_MESSAGE_ENCRYPTER_H_
