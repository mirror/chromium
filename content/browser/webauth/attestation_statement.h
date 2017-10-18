// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_ATTESTATION_STATEMENT_H_
#define CONTENT_BROWSER_WEBAUTH_ATTESTATION_STATEMENT_H_

#include "content/browser/webauth/cbor/cbor_values.h"

namespace content {
namespace authenticator_utils {

// A signed data object containing statements about a credential itself and
// the authenticator that created it.
// Each attestation statement format is defined by the following attributes:
// - Its attestation statement format identifier.
// - The set of attestation types supported by the format.
// - The syntax of an attestation statement produced in this format.
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#cred-attestation.
class AttestationStatement {
 public:
  // The CBOR map data to be added to the attestation object.
  virtual base::flat_map<std::string, CBORValue> GetAsCborData();
  std::string GetFormat();

 protected:
  AttestationStatement(const std::string& format);
  virtual ~AttestationStatement();

 private:
  std::string format_;

  DISALLOW_COPY_AND_ASSIGN(AttestationStatement);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_ATTESTATION_STATEMENT_H_
