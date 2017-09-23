// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_CERTIFICATE_CAST_CRL_H_
#define COMPONENTS_CAST_CERTIFICATE_CAST_CRL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "net/cert/internal/parsed_certificate.h"

namespace net {
class TrustStore;
struct CertPath;
}

namespace cast_certificate {

enum class CastCrlError {
  OK,
  // The certificates of the CRL issuer could not be parsed.
  ERR_ISSUER_PARSE,
  // The CRL issuer is expired.
  ERR_ISSUER_DATE,
  // The CRL issuer could not be verified.
  ERR_ISSUER_VERIFY_GENERIC,
  // The CRL signature could not be verified.
  ERR_SIGNATURE,
  // The CRL is malformed.
  ERR_MALFORMED,
  // The CRL validity date could not be parsed.
  ERR_DATE_PARSE,
  // The CRL is expired.
  ERR_DATE,
  // An internal coding error.
  ERR_UNEXPECTED,
};

// This class represents the CRL information parsed from the binary proto.
class CastCRL {
 public:
  virtual ~CastCRL(){};

  // Verifies the revocation status of a cast device certificate given a chain
  // of X.509 certificates.
  //
  // Inputs:
  // * |chain| the chain of verified certificates, including trust anchor.
  //
  // * |time| is the unix timestamp to use for determining if the certificate
  //   is revoked.
  //
  // Output:
  // Returns true if no certificate in the chain was revoked.
  virtual bool CheckRevocation(const net::CertPath& chain,
                               const base::Time& time) const = 0;
};

// Parses and verifies the CRL used to verify the revocation status of
// Cast device certificates, using the built-in Cast CRL trust anchors.
//
// Inputs:
// * |crl_proto| is a serialized cast_certificate.CrlBundle proto.
// * |time| is the unix timestamp to use for determining if the CRL is valid.
//
// Output:
// Returns the CRL object if success, nullptr otherwise.
// |error| is populated with the appropriate CastCrlError.
std::unique_ptr<CastCRL> ParseAndVerifyCRL(const std::string& crl_proto,
                                           const base::Time& time, CastCrlError* error);

// This is an overloaded version of ParseAndVerifyCRL that allows
// the input of a custom TrustStore.
//
// For production use pass |trust_store| as nullptr to use the production trust
// store.
std::unique_ptr<CastCRL> ParseAndVerifyCRLUsingCustomTrustStore(
    const std::string& crl_proto,
    const base::Time& time,
    net::TrustStore* trust_store, CastCrlError* error);

}  // namespace cast_certificate

#endif  // COMPONENTS_CAST_CERTIFICATE_CAST_CRL_H_
