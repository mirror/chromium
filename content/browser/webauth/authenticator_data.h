// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_

#include "content/browser/webauth/attestation_data.h"

namespace content {
namespace authenticator_utils {

// Flag values
const unsigned char kTestOfUserPresenceFlag = 1 << 0;
const unsigned char kAttestationFlag = 1 << 6;

// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-authenticator-data.
// |rpidhash|flags|counter|attestData|.
class AuthenticatorData {
 public:
  AuthenticatorData(const std::string& relying_party_id,
                    const uint8_t flags,
                    const std::vector<uint8_t>& counter,
                    const std::unique_ptr<AttestationData>& data);
  std::vector<uint8_t> SerializeToByteArray();
  void SetTestOfUserPresenceFlag();
  void SetAttestationFlag();

  virtual ~AuthenticatorData();

 private:
  // RP ID associated with the credential
  std::string relying_party_id_;
  // Flags (bit 0 is the least significant bit) in 1 byte.
  uint8_t flags_;
  // Signature counter, 32-bit unsigned big-endian integer.
  std::vector<uint8_t> counter_;
  std::unique_ptr<AttestationData> attestation_data_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorData);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_
