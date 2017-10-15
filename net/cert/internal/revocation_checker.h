// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_INTERNAL_REVOCATION_CHECKER_H_
#define NET_CERT_INTERNAL_REVOCATION_CHECKER_H_

#include "base/strings/string_piece_forward.h"
#include "net/base/net_export.h"
#include "net/cert/crl_set.h"
#include "net/cert/internal/parsed_certificate.h"

namespace net {

class CertPathErrors;
class CertNetFetcher;
struct CertPathBuilderResultPath;

struct NET_EXPORT_PRIVATE RevocationPolicy {
  RevocationPolicy(bool check_revocation,
                   bool networking_allowed,
                   bool allow_missing_info,
                   bool allow_network_failure);

  // If |check_revocation| is true, then revocation checking (OCSP/CRL) is
  // mandatory. The other properties of RevocationPolicy qualify how revocation
  // checking works.
  bool check_revocation : 1;

  // If |networking_allowed| is true then revocation checking is allowed to
  // issue network requests in order to fetch fresh OCSP/CRL. Otherwise ne
  bool networking_allowed : 1;
  bool allow_missing_info : 1;
  bool allow_network_failure : 1;
};

// |crl_set| and |net_fetcher| may be null.
NET_EXPORT_PRIVATE void CheckCertChainRevocation(
    const CertPathBuilderResultPath& path,
    const RevocationPolicy& policy,
    base::StringPiece stapled_leaf_ocsp_response,
    CertNetFetcher* net_fetcher,
    CertPathErrors* errors);

// Checks the revocation status of a certificate chain using the CRLSet and adds
// revocation errors to |errors|.
//
// Returns the revocation status of the leaf certificate:
//
// * CRLSet::REVOKED if any certificate in the chain is revoked. Also adds a
//   corresponding error for the certificate in |errors|.
//
// * CRLSet::GOOD if the leaf certificate is covered as GOOD by the CRLSet, and
//   none of the intermediates were revoked according to the CRLSet.
//
// * CRLSet::UNKNOWN if none of the certificates are known to be revoked, and
//   the revocation status of leaf certificate was UNKNOWN by the CRLSet.
NET_EXPORT_PRIVATE CRLSet::Result CheckChainRevocationUsingCRLSet(
    const CRLSet* crl_set,
    const ParsedCertificateList& certs,
    CertPathErrors* errors);

}  // namespace net

#endif  // NET_CERT_INTERNAL_REVOCATION_CHECKER_H_
