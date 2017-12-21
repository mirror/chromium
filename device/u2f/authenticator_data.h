// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_AUTHENTICATOR_DATA_H_
#define DEVICE_U2F_AUTHENTICATOR_DATA_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "device/u2f/attested_credential_data.h"

namespace device {

// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-authenticator-data.
class AuthenticatorData {
 public:
  enum class Flag : uint8_t {
    TEST_OF_USER_PRESENCE = 1u << 0,
    ATTESTATION = 1u << 6
  };

  using Flags = uint8_t;

  AuthenticatorData(std::string relying_party_id,
                    Flags flags,
                    std::vector<uint8_t> counter,
                    AttestedCredentialData data);
  virtual ~AuthenticatorData();

  static std::unique_ptr<AuthenticatorData> Create(std::string relying_party_id,
                                                   Flags flags,
                                                   std::vector<uint8_t> counter,
                                                   AttestedCredentialData data);

  // Produces a byte array consisting of:
  // * hash(relying_party_id)
  // * flags
  // * counter
  // * attestation_data.
  std::vector<uint8_t> SerializeToByteArray();

 private:
  // RP ID associated with the credential
  const std::string relying_party_id_;

  // Flags (bit 0 is the least significant bit):
  // [ED | AT | RFU | RFU | RFU | RFU | RFU | UP ]
  //  * Bit 0: Test of User Presence (TUP) result.
  //  * Bits 1-5: Reserved for future use (RFU).
  //  * Bit 6: Attestation data included (AT).
  //  * Bit 7: Extension data included (ED).
  Flags flags_;

  // Signature counter, 32-bit unsigned big-endian integer.
  const std::vector<uint8_t> counter_;
  const AttestedCredentialData attested_data_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorData);
};

}  // namespace device

#endif  // DEVICE_U2F_AUTHENTICATOR_DATA_H_
