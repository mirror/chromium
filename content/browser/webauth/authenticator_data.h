// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

class AttestationData;

namespace {
// Flag values
constexpr uint8_t kTestOfUserPresenceFlag = 1u << 0;
constexpr uint8_t kAttestationFlag = 1u << 6;
}  // namespace

// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-authenticator-data.
class CONTENT_EXPORT AuthenticatorData {
 public:
  enum class Flag : uint8_t { TEST_OF_USER_PRESENCE, ATTESTATION };
  using Flags = uint8_t;

  AuthenticatorData(std::string relying_party_id,
                    uint8_t flags,
                    std::vector<uint8_t> counter,
                    std::unique_ptr<AttestationData> data);
  virtual ~AuthenticatorData();

  static std::unique_ptr<AuthenticatorData> Create(
      std::string client_data_json,
      uint8_t flags,
      std::vector<uint8_t> counter,
      std::unique_ptr<AttestationData> data);

  // Produces a byte array consisting of:
  // * hash(relying_party_id)
  // * flags
  // * counter
  // * attestation_data.
  std::vector<uint8_t> SerializeToByteArray();
  void set_test_of_user_presence_flag() { flags_ |= kTestOfUserPresenceFlag; }
  void set_attestation_flag() { flags_ |= kAttestationFlag; }

 private:
  // RP ID associated with the credential
  const std::string relying_party_id_;

  // Flags (bit 0 is the least significant bit):
  // [ED | AT | RFU | RFU | RFU | RFU | RFU | UP ]
  //  * Bit 0: Test of User Presence (TUP) result.
  //  * Bits 1-5: Reserved for future use (RFU).
  //  * Bit 6: Attestation data included (AT).
  //  * Bit 7: Extension data included (ED).
  uint8_t flags_;

  // Signature counter, 32-bit unsigned big-endian integer.
  const std::vector<uint8_t> counter_;
  const std::unique_ptr<AttestationData> attestation_data_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorData);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_
