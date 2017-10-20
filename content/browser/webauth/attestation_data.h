// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_ATTESTATION_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_ATTESTATION_DATA_H_

#include "content/browser/webauth/public_key.h"

#include <vector>

#include "base/macros.h"
#include "base/values.h"
#include "content/common/content_export.h"

namespace content {

// Information about the authenticator attestation, in bytes, that is appended
// to the end of authenticator_data.
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-attestation-data
class CONTENT_EXPORT AttestationData {
 public:
  static std::unique_ptr<AttestationData> Create(
      const std::vector<uint8_t>& u2f_data,
      const std::vector<uint8_t> aaguid,
      std::unique_ptr<PublicKey> public_key);

  std::vector<uint8_t> GetCredentialId();

  // Produces a byte array consisting of:
  // AAGUID (16 bytes) || Len (2 bytes) || Credential Id (Len bytes) ||
  // Credential Public Key.
  std::vector<uint8_t> SerializeToByteArray();

  virtual ~AttestationData();

 private:
  AttestationData(const std::vector<uint8_t> aaguid,
                  const std::vector<uint8_t> length,
                  const std::vector<uint8_t> credential_id,
                  std::unique_ptr<PublicKey> public_key);

  // The 16-byte AAGUID of the authenticator.
  const std::vector<uint8_t> aaguid_;

  // Big-endian length of the credential (i.e. key handle).
  const std::vector<uint8_t> id_length_;
  const std::vector<uint8_t> credential_id_;
  const std::unique_ptr<PublicKey> public_key_;

  DISALLOW_COPY_AND_ASSIGN(AttestationData);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_ATTESTATION_DATA_H_
