// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/internal/revocation_checker.h"

#include <string>

#include "base/strings/string_piece.h"
#include "crypto/sha2.h"
#include "net/cert/cert_net_fetcher.h"
#include "net/cert/internal/common_cert_errors.h"
#include "net/cert/internal/ocsp.h"
#include "net/cert/internal/parsed_certificate.h"
#include "net/cert/internal/path_builder.h"
#include "net/cert/ocsp_verify_result.h"
#include "url/gurl.h"

namespace net {

namespace {

enum class RevocationDataSource {
  kCRLSet,
  kOcsp,
  kStapledOCSP,
};

void MarkCertificateRevoked(CertErrors* errors, RevocationDataSource source) {
  // TODO: use |source|.
  errors->AddError(cert_errors::kCertificateRevoked);
}

void MarkCertificateRevoked(size_t cert_index,
                            CertPathErrors* errors,
                            RevocationDataSource source) {
  MarkCertificateRevoked(errors->GetErrorsForCert(cert_index), source);
}

// TODO(eroman): Make the verification time an input.
void CheckCertRevocation(const ParsedCertificate* cert,
                         const ParsedCertificate* issuer_cert,
                         const RevocationPolicy& policy,
                         base::StringPiece stapled_ocsp_response,
                         CertNetFetcher* net_fetcher,
                         CertErrors* cert_errors,
                         bool* done) {
  // Check using stapled OCSP, if available.
  if (!stapled_ocsp_response.empty() && issuer_cert) {
    // TODO(eroman): CheckOCSP() re-parses the certificates, perhaps just pass
    //               ParsedCertificate into it.
    OCSPVerifyResult::ResponseStatus response_details;
    OCSPRevocationStatus ocsp_status =
        CheckOCSP(stapled_ocsp_response, cert->der_cert().AsStringPiece(),
                  issuer_cert->der_cert().AsStringPiece(), base::Time::Now(),
                  &response_details);

    // TODO(eroman): Save the stapled OCSP response to cache.
    switch (ocsp_status) {
      case OCSPRevocationStatus::REVOKED:
        *done = true;
        MarkCertificateRevoked(cert_errors, RevocationDataSource::kStapledOCSP);
        return;
      case OCSPRevocationStatus::GOOD:
        return;
      case OCSPRevocationStatus::UNKNOWN:
        // TODO(eroman): If the OCSP response was invalid, should we keep
        //               looking or fail?
        break;
    }
  }

  // TODO(eroman): Check CRL.

  if (!policy.check_revocation) {
    // TODO(eroman): Should still check CRL/OCSP caches.
    return;
  }

  bool found_revocation_info = false;
  bool failed_network_fetch = false;

  // Check OCSP.
  if (cert->has_authority_info_access()) {
    // Try each of the OCSP URIs
    for (const auto& ocsp_uri : cert->ocsp_uris()) {
      // Only consider http:// URLs (https:// could create a circular
      // dependency).
      GURL responder_url(ocsp_uri);
      if (!responder_url.is_valid() || !responder_url.SchemeIsHTTPOrHTTPS())
        continue;

      found_revocation_info = true;

      if (!policy.networking_allowed || !net_fetcher)
        continue;

      // TODO(eroman): Duplication of work if there are multiple URLs to try.
      // TODO(eroman): Are there cases where we would need to POST instead?
      GURL get_url = CreateOCSPGetURL(cert, issuer_cert, responder_url);
      if (!get_url.is_valid()) {
        // A failure here is unexpected, but could happen from BoringSSL.
        continue;
      }

      // Fetch it over network.
      //
      // TODO(eroman): Issue POST instead of GET if request is larger than 255
      //               bytes?
      // TODO(eroman): Improve interplay with HTTP cache.
      //
      // TODO(eroman): Bound the maximum time allowed spent doing network
      // requests.
      std::unique_ptr<CertNetFetcher::Request> net_ocsp_request =
          net_fetcher->FetchOcsp(get_url, CertNetFetcher::DEFAULT,
                                 CertNetFetcher::DEFAULT);

      Error net_error;
      std::vector<uint8_t> ocsp_response_bytes;
      net_ocsp_request->WaitForResult(&net_error, &ocsp_response_bytes);

      if (net_error != OK) {
        failed_network_fetch = true;
        continue;
      }

      OCSPVerifyResult::ResponseStatus response_details;

      OCSPRevocationStatus ocsp_status = CheckOCSP(
          base::StringPiece(
              reinterpret_cast<const char*>(ocsp_response_bytes.data()),
              ocsp_response_bytes.size()),
          cert->der_cert().AsStringPiece(),
          issuer_cert->der_cert().AsStringPiece(), base::Time::Now(),
          &response_details);

      switch (ocsp_status) {
        case OCSPRevocationStatus::REVOKED:
          *done = true;
          MarkCertificateRevoked(cert_errors, RevocationDataSource::kOcsp);
          return;
        case OCSPRevocationStatus::GOOD:
          return;
        case OCSPRevocationStatus::UNKNOWN:
          break;
      }
    }
  }

  // Reaching here means that revocation checking was inconclusive. Determine
  // whether failure to complete revocation checking constitutes an error.

  if (!found_revocation_info && policy.allow_missing_info) {
    // If the certificate lacked any (recognized) revocation mechanisms, and the
    // policy permits it, consider revocation checking a success.
    return;
  }

  if (!found_revocation_info && !policy.allow_missing_info) {
    // If the certificate lacked any (recognized) revocation mechanisms, and
    // the policy forbids it, fail revocation checking.
    cert_errors->AddError(cert_errors::kNoRevocationMechanism);
    *done = true;
    return;
  }

  // In soft-fail mode permit failures due to network errors.
  if (failed_network_fetch && policy.allow_network_failure &&
      policy.networking_allowed) {
    return;
  }

  // Otherwise revocation failed for a reason not permitted by the policy.
  cert_errors->AddError(cert_errors::kUnableToCheckRevocation);
  *done = true;
}

}  // namespace

RevocationPolicy::RevocationPolicy(bool check_revocation,
                                   bool networking_allowed,
                                   bool allow_missing_info,
                                   bool allow_network_failure)
    : check_revocation(check_revocation),
      networking_allowed(networking_allowed),
      allow_missing_info(allow_missing_info),
      allow_network_failure(allow_network_failure) {}

// This method checks whether a certificate chain has been revoked, and if
// so adds errors to the affected certificates.
//
// Revocation checks proceed in the order:
//
//  (1) Best-effort offline revocation checks.
//  (2) Online revocation checks, if required.
//
//  The offline revocation checks are applied irrespective of any flags when
//  their data is available.
//
//    * CRLSet - This provides revocations for high value certificates
//               (including trust anchors)
//    * Stapled OCSP - When provided by TLS covers the leaf certificate
//    * Cached OCSP responses
//    * Cached CRLs
//
//  If any certificates were found to be revoked, processing ends.
//
//  The offline OCSP/CRL checks may have determined some subset of
//  certificates to be unrevoked.
//
//  If all certificates were affirmatively unrevoked, then processing ends.
//  Otherwise online revocation methods are used to determine the status of
//  the remaining certificates, if online revocation checking is enabled.
//
//  There are a number of situations  requested.
//
//  Online revocation checking is enabled as follows:
//
//    (a) If |flags & VERIFY_EV_CERT| and the leaf certificate has a possible
//        EV policy OID, then hard-fail revocation checking will be
//        enabled as part of EV validation.
//
//    (b) If |flags & VERIFY_REV_CHECKING_REQUIRED_LOCAL_ANCHORS| and the
//        chain's root is a locally installed trust anchor, then hard-fail
//        revocation checking will be enabled.
//
//    (c) If |flags & VERIFY_REV_CHECKING_ENABLED| then soft-fail revocation
//        checking will be enabled.
//
//        TODO(eroman): The NSS implementation uses soft-fail rather than
//                      hard-fail. Should we match?
//
//        TODO(eroman): The NSS implementation additionally requires
//                      |flags & VERIFY_CERT_IO_ENABLED|. Should we match?
//
//        TODO(eroman): VERIFY_REV_CHECKING_ENABLED_EV_ONLY
void CheckCertChainRevocation(const CertPathBuilderResultPath& path,
                              const RevocationPolicy& policy,
                              base::StringPiece stapled_leaf_ocsp_response,
                              CertNetFetcher* net_fetcher,
                              CertPathErrors* errors) {
  const ParsedCertificateList& certs = path.certs;

  // Next check each certificate for revocation using OCSP/CRL. Checks proceed
  // from the root certificate towards the leaf certificate. Revocation errors
  // are added to |errors|. This loop may short-circuit before having processed
  // all certificates by setting |done=true|.
  bool done = false;
  for (size_t reverse_i = 0; reverse_i < certs.size() && !done; ++reverse_i) {
    size_t i = certs.size() - reverse_i - 1;
    const ParsedCertificate* cert = certs[i].get();
    const ParsedCertificate* issuer_cert =
        i + 1 < certs.size() ? certs[i + 1].get() : nullptr;

    // Trust anchors bypass OCSP/CRL revocation checks. (The only way to revoke
    // trust anchors is via CRLSet or the built-in SPKI blacklist).
    if (reverse_i == 0 && path.last_cert_trust.IsTrustAnchor())
      continue;

    // TODO(eroman): Plumb stapled OCSP for non-leaf certificates from TLS?
    base::StringPiece stapled_ocsp =
        (i == 0) ? stapled_leaf_ocsp_response : base::StringPiece();

    // Check whether this certificate's revocation status complies with the
    // policy. Calling this function may add errors, or set |done=true|.
    CheckCertRevocation(cert, issuer_cert, policy, stapled_ocsp, net_fetcher,
                        errors->GetErrorsForCert(i), &done);
  }
}

CRLSet::Result CheckChainRevocationUsingCRLSet(
    const CRLSet* crl_set,
    const ParsedCertificateList& certs,
    CertPathErrors* errors) {
  // Iterate from the root certificate towards the leaf (the root certificate is
  // also checked for revocation by CRLSet).
  std::string issuer_spki_hash;
  for (size_t reverse_i = 0; reverse_i < certs.size(); ++reverse_i) {
    size_t i = certs.size() - reverse_i - 1;
    const ParsedCertificate* cert = certs[i].get();

    // True if |cert| is the root of the chain.
    const bool is_root = reverse_i == 0;
    // True if |cert| is the leaf certificate of the chain.
    const bool is_target = i == 0;

    // Check for revocation using the certificate's SPKI.
    std::string spki_hash =
        crypto::SHA256HashString(cert->tbs().spki_tlv.AsStringPiece());
    CRLSet::Result result = crl_set->CheckSPKI(spki_hash);

    // Check for revocation using the certificate's serial number and issuer's
    // SPKI.
    if (result != CRLSet::REVOKED && !is_root) {
      result = crl_set->CheckSerial(cert->tbs().serial_number.AsStringPiece(),
                                    issuer_spki_hash);
    }

    // Prepare for the next iteration.
    issuer_spki_hash = std::move(spki_hash);

    switch (result) {
      case CRLSet::REVOKED:
        MarkCertificateRevoked(i, errors, RevocationDataSource::kCRLSet);
        return CRLSet::Result::REVOKED;
      case CRLSet::UNKNOWN:
        // If the status is unknown, advance to the subordinate certificate.
        break;
      case CRLSet::GOOD:
        if (is_target && !crl_set->IsExpired()) {
          // If the target is covered by the CRLSet and known good, consider
          // the entire chain to be valid (even though the revocation status
          // of the intermediates may have been UNKNOWN).
          //
          // Only the leaf certificate is considered for coverage because some
          // intermediates have CRLs with no revocations (after filtering) and
          // those CRLs are pruned from the CRLSet at generation time.
          return CRLSet::Result::GOOD;
        }
        break;
    }
  }

  // If no certificate was revoked, and the target was not known good, then
  // the revocation status is still unknown.
  return CRLSet::Result::UNKNOWN;
}

}  // namespace net
