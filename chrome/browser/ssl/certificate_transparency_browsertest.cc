// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base64.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/net/sth_distributor_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/certificate_transparency/single_tree_tracker.h"
#include "components/certificate_transparency/tree_state_tracker.h"
#include "net/cert/ct_policy_status.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/cert/signed_tree_head.h"
#include "net/cert/sth_distributor.h"
#include "net/dns/mock_host_resolver.h"
#include "net/quic/test_tools/crypto_test_utils.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/test_data_directory.h"
#include "net/tools/quic/quic_simple_server.h"

namespace {

using net::ct::DigitallySigned;
using net::ct::SignedTreeHead;

bool DecodeRootHash(base::StringPiece base64_data,
                    char hash_out[net::ct::kSthRootHashLength]) {
  std::string root_hash;
  if (!base::Base64Decode(base64_data, &root_hash))
    return false;

  root_hash.copy(hash_out, net::ct::kSthRootHashLength);
  return true;
}

bool DecodeDigitallySigned(base::StringPiece base64_data,
                           DigitallySigned* sig_out) {
  std::string data;
  if (!base::Base64Decode(base64_data, &data))
    return false;

  base::StringPiece data_ptr = data;
  if (!net::ct::DecodeDigitallySigned(&data_ptr, sig_out))
    return false;

  return true;
}

bool BuildSignedTreeHead(base::Time timestamp,
                         uint64_t tree_size,
                         base::StringPiece root_hash_base64,
                         base::StringPiece signature_base64,
                         base::StringPiece log_id_base64,
                         net::ct::SignedTreeHead* sth_out) {
  sth_out->version = SignedTreeHead::V1;
  sth_out->timestamp = timestamp;
  sth_out->tree_size = tree_size;

  return DecodeRootHash(root_hash_base64, sth_out->sha256_root_hash) &&
         DecodeDigitallySigned(signature_base64, &sth_out->signature) &&
         base::Base64Decode(log_id_base64, &sth_out->log_id);
}

enum class ServerType {
  HTTPS,
  QUIC,
};

// A test fixture that allows tests to intercept certificate verification.
class CertificateTransparencyBrowserTest
    : public CertVerifierBrowserTest,
      public ::testing::WithParamInterface<ServerType> {
 public:
  CertificateTransparencyBrowserTest()
      : CertVerifierBrowserTest(),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

 protected:
  // Sets up the mock cert verifier to swap the HTTP server's certificate for a
  // different one during the verification process. Verification will return
  // successfully, indicating that this replacement certificate is valid and
  // issued by a known root (CT checks are skipped for private roots).
  // This is useful because the default server certificate does not contain
  // SCTs, and this also makes it possible to use certificates for which the
  // private key is not available.
  void SwapCert(scoped_refptr<net::X509Certificate> server_cert,
                const std::string& replacement_cert_name) {
    base::ScopedAllowBlockingForTesting allow_blocking_for_loading_cert;

    net::CertVerifyResult verify_result;
    verify_result.verified_cert = net::CreateCertificateChainFromFile(
        net::GetTestCertsDirectory(), replacement_cert_name,
        net::X509Certificate::FORMAT_PEM_CERT_SEQUENCE);
    ASSERT_TRUE(verify_result.verified_cert);
    verify_result.is_issued_by_known_root = true;
    verify_result.cert_status = 0;
    mock_cert_verifier()->AddResultForCert(server_cert, verify_result, net::OK);
  }

  net::EmbeddedTestServer https_server_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CertificateTransparencyBrowserTest);
};

// Tests that wiring is correctly setup to pass Signed Certificate Timestamps
// (SCTs) through from SSL to the CT verification and inclusion checking code.
// This is assessed by checking that histograms are correctly populated.
IN_PROC_BROWSER_TEST_F(CertificateTransparencyBrowserTest, SSLTest) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {certificate_transparency::kCTLogAuditing}, {});

  // Provide an STH from Google's Pilot log that can be used to prove inclusion
  // for an SCT later in the test.
  SignedTreeHead pilot_sth;
  ASSERT_TRUE(BuildSignedTreeHead(
      base::Time::FromJsTime(1512419914170), 181871752,
      "bvgljSy3Yg32Y6J8qL5WmUA3jn2WnOrEFDqxD0AxUvs=",
      "BAMARjBEAiAwEXve2RBk3XkUR+6nACSETTgzKFaEeginxuj5U9BI/"
      "wIgBPuQS5ACxsro6TtpY4bQyE6WlMdcSMiMd/SSGraOBOg=",
      "pLkJkLQYWBSHuxOizGdwCjw1mAT5G9+443fNDsgN3BA=", &pilot_sth));
  chrome_browser_net::GetGlobalSTHDistributor()->NewSTHObserved(pilot_sth);

  // Provide an STH from Google's Aviator log that is not recent enough to prove
  // inclusion for an SCT later in the test.
  SignedTreeHead aviator_sth;
  ASSERT_TRUE(BuildSignedTreeHead(
      base::Time::FromJsTime(1442652106945), 8502329,
      "bfG+gWZcHl9fqtNo0Z/uggs8E5YqGOtJQ0Z5zVZDRxI=",
      "BAMARjBEAiA6elcNQoShmKLHj/"
      "IA649UIbaQtWJEpj0Eot0q7G6fEgIgYChb7U6Reuvt0nO5PionH+3UciOxKV3Cy8/"
      "eq59lSYY=",
      "aPaY+B9kgr46jO65KB1M/HFRXWeT1ETRCmesu09P+8Q=", &aviator_sth));
  chrome_browser_net::GetGlobalSTHDistributor()->NewSTHObserved(aviator_sth);

  // TODO(robpercival): Override CT log list to ensure it includes Google's
  // Pilot and Aviator CT logs, as well as DigiCert's CT log.

  ASSERT_TRUE(https_server_.Start());
  // Swap the normal test cert for one that contains 3 SCTs, which fulfil
  // Chrome's CT policy.
  SwapCert(https_server_.GetCertificate(), "comodo-chain.pem");
  const int kNumSCTs = 3;

  base::HistogramTester histograms;

  // Navigate to a test server URL, which should trigger a CT inclusion check
  // thanks to the cert mock_cert_verifier swaps in.
  ui_test_utils::NavigateToURL(browser(), https_server_.GetURL("/"));
  base::RunLoop().RunUntilIdle();

  // Expect 3 SCTs in this connection.
  histograms.ExpectUniqueSample("Net.CertificateTransparency.SCTsPerConnection",
                                kNumSCTs, 1);

  // Expect that the SCTs were embedded in the certificate.
  histograms.ExpectUniqueSample(
      "Net.CertificateTransparency.SCTOrigin",
      static_cast<int>(net::ct::SignedCertificateTimestamp::SCT_EMBEDDED),
      kNumSCTs);

  // The certificate contains a sufficient number and diversity of SCTs.
  // 2 from Google CT logs (Pilot, Aviator) and 1 from a non-Google CT log
  // (DigiCert).
  histograms.ExpectUniqueSample(
      "Net.CertificateTransparency.ConnectionComplianceStatus2.SSL",
      static_cast<int>(
          net::ct::CTPolicyCompliance::CT_POLICY_COMPLIES_VIA_SCTS),
      1);

  // The Pilot SCT should be eligible for inclusion checking, because a recent
  // enough Pilot STH is available.
  histograms.ExpectBucketCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT",
      certificate_transparency::CAN_BE_CHECKED, 1);
  // The Aviator SCT should not be eligible for inclusion checking, because
  // there is not a recent enough Aviator STH available.
  histograms.ExpectBucketCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT",
      certificate_transparency::NEWER_STH_REQUIRED, 1);
  // The DigiCert SCT should not be eligible for inclusion checking, because
  // there is no DigiCert STH available.
  histograms.ExpectBucketCount(
      "Net.CertificateTransparency.CanInclusionCheckSCT",
      certificate_transparency::VALID_STH_REQUIRED, 1);
}

}  // namespace
