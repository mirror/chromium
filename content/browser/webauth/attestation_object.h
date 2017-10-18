// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_ATTESTATION_OBJECT_H_
#define CONTENT_BROWSER_WEBAUTH_ATTESTATION_OBJECT_H_

#include "content/browser/webauth/attestation_statement.h"
#include "content/browser/webauth/authenticator_data.h"

namespace content {
namespace authenticator_utils {

constexpr char kAuthKey[] = "authData";
constexpr char kFmtKey[] = "fmt";
constexpr char kAttestationKey[] = "attStmt";

// Object containing the authenticator-provided attestation every time
// a credential is created, per
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation
//
// The syntax of an attestation object is defined as follows:
//
// attObj = {
//   authData: bytes,
//   $$attStmtType
// }
// attStmtTemplate = (
//   fmt: text,
//   attStmt: bytes
// )
// ; Every attestation statement format must have the above fields
// attStmtTemplate .within $$attStmtType
class AttestationObject {
 public:
  AttestationObject(const std::unique_ptr<AuthenticatorData>& data,
                    const std::unique_ptr<AttestationStatement>& statement);

  std::vector<uint8_t> GetCborEncodedByteArray();

  virtual ~AttestationObject();

 private:
  std::unique_ptr<AuthenticatorData> authenticator_data_;
  std::unique_ptr<AttestationStatement> attestation_statement_;

  DISALLOW_COPY_AND_ASSIGN(AttestationObject);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_ATTESTATION_OBJECT_H_
