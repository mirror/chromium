// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_DATA_H_

#include "content/browser/webauth/attestation_data.h"

#include "content/common/content_export.h"

namespace content {

namespace {
// Flag values
const unsigned char kTestOfUserPresenceFlag = 1 << 0;
const unsigned char kAttestationFlag = 1 << 6;
}  // namespace

// Data about the authenticator that will be CBOR-encoded into an
// attestation object.
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#sec-authenticator-data.
class CONTENT_EXPORT AuthenticatorData {
 public:
  static std::unique_ptr<AuthenticatorData> Create(
      const std::string client_data_json,
      const uint8_t flags,
      const std::vector<uint8_t> counter,
      std::unique_ptr<AttestationData> data);

  // Produces a byte array consisting of:
  // hash(relying_party_id)||flags||counter||attestation_data.
  std::vector<uint8_t> SerializeToByteArray();
  void SetTestOfUserPresenceFlag();
  void SetAttestationFlag();

  virtual ~AuthenticatorData();

 protected:
  AuthenticatorData(const std::string relying_party_id,
                    const uint8_t flags,
                    const std::vector<uint8_t> counter,
                    std::unique_ptr<AttestationData> data);

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
