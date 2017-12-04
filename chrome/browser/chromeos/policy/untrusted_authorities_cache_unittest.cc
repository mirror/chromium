// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/untrusted_authorities_cache.h"

#include <cert.h>
#include <certdb.h>
#include <secitem.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/onc/onc_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/cert/internal/cert_errors.h"
#include "net/cert/internal/parse_certificate.h"
#include "net/cert/pem_tokenizer.h"
#include "net/der/input.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

class UntrustedAuthoritiesCacheTest : public testing::Test {
 public:
  UntrustedAuthoritiesCacheTest() {}
  ~UntrustedAuthoritiesCacheTest() override {}

  void SetUp() override {
    chromeos::LoginState::Initialize();
    untrusted_authorities_cache_ = std::make_unique<UntrustedAuthoritiesCache>(
        content::BrowserThread::GetTaskRunnerForThread(
            content::BrowserThread::IO));
  }

  void TearDown() override { chromeos::LoginState::Shutdown(); }

 protected:
  void UpdateUntrustedAuthoritiesFromFiles(
      std::vector<base::FilePath> pem_cert_files) {
    std::vector<std::string> x509_authority_certs;
    for (const auto& pem_cert_file : pem_cert_files) {
      std::string x509_authority_cert;
      base::ReadFileToString(pem_cert_file, &x509_authority_cert);
      x509_authority_certs.emplace_back(std::move(x509_authority_cert));
    }

    untrusted_authorities_cache_->UpdateUntrustedAuthoritiesFromPolicy(
        x509_authority_certs);
    base::RunLoop().RunUntilIdle();
  }

  void IsCertificateAvailable(const base::FilePath& pem_file_path,
                              bool* out_available) {
    std::string file_data;
    ASSERT_TRUE(base::ReadFileToString(pem_file_path, &file_data));

    std::vector<std::string> pem_headers;
    pem_headers.push_back("CERTIFICATE");
    net::PEMTokenizer pem_tokenizer(file_data, pem_headers);
    ASSERT_TRUE(pem_tokenizer.GetNext());
    const std::string& cert_data = pem_tokenizer.data();

    // Parsing the certificate.
    net::der::Input tbs_certificate_tlv;
    net::der::Input signature_algorithm_tlv;
    net::der::BitString signature_value;
    net::CertErrors errors;
    ASSERT_TRUE(net::ParseCertificate(
        net::der::Input(&cert_data), &tbs_certificate_tlv,
        &signature_algorithm_tlv, &signature_value, &errors));
    net::ParsedTbsCertificate tbs;
    net::ParseCertificateOptions options;
    options.allow_invalid_serial_numbers = true;
    ASSERT_TRUE(
        net::ParseTbsCertificate(tbs_certificate_tlv, options, &tbs, nullptr));
    net::der::Input subject = tbs.subject_tlv;

    SECItem subject_item;
    subject_item.len = subject.Length();
    subject_item.data = const_cast<unsigned char*>(subject.UnsafeData());

    net::ScopedCERTCertificate found_cert(
        CERT_FindCertByName(CERT_GetDefaultCertDB(), &subject_item));
    *out_available = (found_cert != nullptr);
  }

  void LoginToRegularUser() {
    chromeos::LoginState::Get()->SetLoggedInState(
        chromeos::LoginState::LOGGED_IN_ACTIVE,
        chromeos::LoginState::LOGGED_IN_USER_REGULAR);
    base::RunLoop().RunUntilIdle();
  }

 private:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;

  std::unique_ptr<UntrustedAuthoritiesCache> untrusted_authorities_cache_;

  DISALLOW_COPY_AND_ASSIGN(UntrustedAuthoritiesCacheTest);
};

TEST_F(UntrustedAuthoritiesCacheTest, CertAvailableBeforeUserSignin) {
  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});
  bool cert_available = false;
  ASSERT_NO_FATAL_FAILURE(
      IsCertificateAvailable(cert_file_path, &cert_available));
  EXPECT_TRUE(cert_available);
}

TEST_F(UntrustedAuthoritiesCacheTest, OtherCertNotAvailable) {
  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});
  base::FilePath other_cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_2_ca.pem"));
  bool cert_available = false;
  ASSERT_NO_FATAL_FAILURE(
      IsCertificateAvailable(other_cert_file_path, &cert_available));
  EXPECT_FALSE(cert_available);
}

TEST_F(UntrustedAuthoritiesCacheTest, CetNotAvailableAfterUserSignin) {
  LoginToRegularUser();

  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});
  bool cert_available = false;
  ASSERT_NO_FATAL_FAILURE(
      IsCertificateAvailable(cert_file_path, &cert_available));
  EXPECT_FALSE(cert_available);
}

// TOOD Should this work??
TEST_F(UntrustedAuthoritiesCacheTest, CetNotAvailableAnymoreAfterUserSignin) {
  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});

  LoginToRegularUser();
  bool cert_available = false;
  ASSERT_NO_FATAL_FAILURE(
      IsCertificateAvailable(cert_file_path, &cert_available));
  EXPECT_FALSE(cert_available);
}

}  // namespace
}  // namespace policy
