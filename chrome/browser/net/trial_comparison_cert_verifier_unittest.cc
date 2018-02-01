// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/trial_comparison_cert_verifier.h"

#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/x509_certificate.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Mock CertVerifyProc that sets the CertVerifyResult to a given value for
// all certificates that are Verify()'d
class MockCertVerifyProc : public net::CertVerifyProc {
 public:
  explicit MockCertVerifyProc(const int result_error,
                              const net::CertVerifyResult& result)
      : result_error_(result_error), result_(result) {}
  // CertVerifyProc implementation:
  bool SupportsAdditionalTrustAnchors() const override { return false; }
  bool SupportsOCSPStapling() const override { return false; }
  void WaitForVerifyCall() { verify_called_.WaitForResult(); }

 protected:
  ~MockCertVerifyProc() override = default;

 private:
  int VerifyInternal(net::X509Certificate* cert,
                     const std::string& hostname,
                     const std::string& ocsp_response,
                     int flags,
                     net::CRLSet* crl_set,
                     const net::CertificateList& additional_trust_anchors,
                     net::CertVerifyResult* verify_result) override;

  const int result_error_;
  const net::CertVerifyResult result_;
  TestClosure verify_called_;

  DISALLOW_COPY_AND_ASSIGN(MockCertVerifyProc);
};

int MockCertVerifyProc::VerifyInternal(
    net::X509Certificate* cert,
    const std::string& hostname,
    const std::string& ocsp_response,
    int flags,
    net::CRLSet* crl_set,
    const net::CertificateList& additional_trust_anchors,
    net::CertVerifyResult* verify_result) {
  *verify_result = result_;
  //verify_result->verified_cert = cert;
  return result_error_;
}

}  // namespace

class TrialComparisonCertVerifierTest : public testing::Test {
 public:
  TrialComparisonCertVerifierTest()
      // XXX thread bundle flags
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {}

  void SetUp() override {
    // XXX setup prefs
    // profile_.GetTestingPrefService()
    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    ASSERT_TRUE(profile_manager_->SetUp());
  }

  void Initialize() {
    context_->set_client_socket_factory(&socket_factory_);
    context_->set_network_delegate(network_delegate_.get());
    context_->Init();
  }

  net::TestURLRequestContext* context() { return context_.get(); }
  net::NetworkDelegate* network_delegate() { return network_delegate_.get(); }
  net::MockClientSocketFactory* socket_factory() { return &socket_factory_; }

  ChromeNetworkDelegate* chrome_network_delegate() {
    return network_delegate_.get();
  }


 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  TestingProfile profile_;
  /*
  BooleanPrefMember enable_referrers_;
  std::unique_ptr<ChromeNetworkDelegate> network_delegate_;
  net::MockClientSocketFactory socket_factory_;
  std::unique_ptr<net::TestURLRequestContext> context_;*/
};

TEST_F(TrialComparisonCertVerifierTest, SameResult) {
  scoped_refptr<net::X509Certificate> cert_chain = CreateCertificateChainFromFile(
      GetTestCertsDirectory(),
      "multi-root-chain1.pem" net::X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert_chain);
  scoped_refptr<net::X509Certificate> cert =
      net::X509Certificate::CreateFromBuffer(cert_chain->cert_buffer(), {});
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = cert_chain;
  scoped_refptr<CertVerifyProc> verify_proc1 =
      new MockCertVerifyProc(net::OK, dummy_result);
  scoped_refptr<CertVerifyProc> verify_proc2 =
      new MockCertVerifyProc(net::OK, dummy_result);

  // XXX override reporting service
  // XXX set trial & prefs
  TrialComparisonCertVerifier verifier(verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(cert, "127.0.0.1", 0 /* flags */,
                                          std::string() /* ocsp_response */,
                                          {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error = verifier.Verify(params, nullptr /* crl_set */, &result, callback,
                              &request, netlog);
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  // XXX wrong thred
  verify_proc2.WaitForVerifyCall();

  // XXX 

}

// XXX TODO more tests:
// SBERPrefDisabled
// NotScout
// TrialNotActive
// Incognito
// DifferentCertChains
// PrimaryVerifierError
// SecondaryVerifierError
// BothVerifiersDifferentErrors
// Cancellation ...
