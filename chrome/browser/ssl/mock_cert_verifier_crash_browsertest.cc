// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_features.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"

namespace {

class MockCertVerifierBrowserTest : public CertVerifierBrowserTest {
 public:
  MockCertVerifierBrowserTest()
      : CertVerifierBrowserTest(),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitWithFeatures({features::kNetworkService}, {});
  }

 protected:
  net::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList feature_list_;

  DISALLOW_COPY_AND_ASSIGN(MockCertVerifierBrowserTest);
};

IN_PROC_BROWSER_TEST_F(MockCertVerifierBrowserTest, ShouldNotCrash) {
  https_server_.SetSSLConfig(
      net::test_server::EmbeddedTestServer::CERT_COMMON_NAME_IS_DOMAIN);
  ASSERT_TRUE(https_server_.Start());

  net::CertVerifyResult verify_result;
  verify_result.is_issued_by_known_root = true;
  verify_result.cert_status = 0;

  {
    base::ScopedAllowBlockingForTesting allow_blocking_for_loading_cert;

    verify_result.verified_cert = net::CreateCertificateChainFromFile(
        net::GetTestCertsDirectory(), "twitter-chain.pem",
        net::X509Certificate::FORMAT_PEM_CERT_SEQUENCE);
    ASSERT_TRUE(verify_result.verified_cert);
  }

  mock_cert_verifier()->AddResultForCert(https_server_.GetCertificate(),
                                         verify_result, net::OK);

  ui_test_utils::NavigateToURL(browser(), https_server_.GetURL("/"));
}

}  // namespace
