// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_ATTESTATION_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_ATTESTATION_DATA_H_

#include "content/browser/webauth/public_key.h"

namespace content {
namespace authenticator_utils {

// This consists of the AAGUID (16 bytes), Length (2 bytes),
// Credential ID (L bytes), and Credential Public Key (65 bytes)
class AttestationData {
 public:
  AttestationData(const std::vector<uint8_t>& aaguid,
                  const std::vector<uint8_t>& length,
                  const std::vector<uint8_t>& credential_id,
                  const std::unique_ptr<PublicKey>& public_key);
  std::vector<uint8_t> SerializeToByteArray();

  virtual ~AttestationData();

 private:
  // The 16-byte AAGUID of the authenticator.
  std::vector<uint8_t> aaguid_;

  // Big-endian length of the credential (i.e. key handle).
  std::vector<uint8_t> id_length_;
  std::vector<uint8_t> credential_id_;
  std::unique_ptr<PublicKey> public_key_;

  DISALLOW_COPY_AND_ASSIGN(AttestationData);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_ATTESTATION_DATA_H_
