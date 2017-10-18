// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_FIDO_ATTESTATION_STATEMENT_H_
#define CONTENT_BROWSER_WEBAUTH_FIDO_ATTESTATION_STATEMENT_H_

#include "content/browser/webauth/attestation_statement.h"

namespace content {
namespace authenticator_utils {

// The attestation statement format used with FIDO U2F authenticators
// using the formats defined in  [FIDO-U2F-Message-Formats].
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#fido-u2f-attestation
//
// The syntax of a FIDO U2F attestation statement is defined as follows:
//
//    $$attStmtType //= (
//                          fmt: "fido-u2f",
//                          attStmt: u2fStmtFormat
//                      )
//
//    u2fStmtFormat = {
//                        x5c: [ attestnCert: bytes, * (caCert: bytes) ],
//                        sig: bytes
//                    }
class FidoAttestationStatement : public AttestationStatement {
 public:
  FidoAttestationStatement(const std::vector<uint8_t>& signature,
                           const std::vector<std::vector<uint8_t>>& x5c);
  base::flat_map<std::string, CBORValue> GetAsCborData() override;

  ~FidoAttestationStatement() override;

 private:
  std::vector<uint8_t> signature_;
  std::vector<std::vector<uint8_t>> x509_certs_;

  DISALLOW_COPY_AND_ASSIGN(FidoAttestationStatement);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_FIDO_ATTESTATION_STATEMENT_H_
