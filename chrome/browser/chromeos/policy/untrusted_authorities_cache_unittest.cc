// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/untrusted_authorities_cache.h"

#include <cert.h>
#include <certdb.h>
#include <secitem.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chromeos/login/login_state.h"
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
  // Reads the certificates from |pem_cert_files|, assuming that each file
  // ocntains one CERTIFICATE block. Passes all certificates to
  // |untrusted_authorities_cache_|.
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

  // Checks if the certificate stored in |pem_cert_file| can be find in the
  // default NSS certificate database using CERT_FindCertByName.
  // Stores the result in *|out_available|.
  // Note: This funcion uses ASSERT_ macros, so the caller must verify for
  // failures after it returns.
  void CheckIsCertificateAvailable(const base::FilePath& pem_cert_file,
                                   bool* out_available) {
    std::string cert_contents_buffer;
    net::der::Input subject;
    ASSERT_NO_FATAL_FAILURE(GetCertificateSubjectDN(
        pem_cert_file, &cert_contents_buffer, &subject));

    SECItem subject_item;
    subject_item.len = subject.Length();
    subject_item.data = const_cast<unsigned char*>(subject.UnsafeData());

    net::ScopedCERTCertificate found_cert(
        CERT_FindCertByName(CERT_GetDefaultCertDB(), &subject_item));
    *out_available = static_cast<bool>(found_cert);
  }

  // Determines the subject DN of the certificate stored in
  // |pem_cert_file|. Stores the result in *|out_subject|.
  // the der::Input data structure contains unowned pointers into the
  // certificate data buffer. The caller must pass a buffer in
  // |cert_contents_buffer| and ensure to only use *|out_subject| while
  // *|cert_contents_buffer| is in scope.
  // Note: This funcion uses ASSERT_ macros, so the caller must verify for
  // failures after it returns.
  void GetCertificateSubjectDN(const base::FilePath& pem_cert_file,
                               std::string* cert_contents_buffer,
                               net::der::Input* out_subject) {
    std::string file_data;
    ASSERT_TRUE(base::ReadFileToString(pem_cert_file, &file_data));

    std::vector<std::string> pem_headers;
    pem_headers.push_back("CERTIFICATE");
    net::PEMTokenizer pem_tokenizer(file_data, pem_headers);
    ASSERT_TRUE(pem_tokenizer.GetNext());
    *cert_contents_buffer = pem_tokenizer.data();

    // Parsing the certificate.
    net::der::Input tbs_certificate_tlv;
    net::der::Input signature_algorithm_tlv;
    net::der::BitString signature_value;
    net::CertErrors errors;
    ASSERT_TRUE(net::ParseCertificate(
        net::der::Input(cert_contents_buffer), &tbs_certificate_tlv,
        &signature_algorithm_tlv, &signature_value, &errors));

    net::ParsedTbsCertificate tbs;
    net::ParseCertificateOptions options;
    options.allow_invalid_serial_numbers = true;
    ASSERT_TRUE(
        net::ParseTbsCertificate(tbs_certificate_tlv, options, &tbs, nullptr));
    *out_subject = tbs.subject_tlv;
  }

  // Simluates that a user signs in.
  void LoginRegularUser() {
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

// Checks that a certificate made available through the
// UntrustedAuthoritiesCache can be found using CERT_FindCertByName.
// Additionally, checks that the certificate is not bot available after a user
// signs in anymore.
TEST_F(UntrustedAuthoritiesCacheTest, CertAvailableBeforeUserSignin) {
  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});

  bool cert_available_before_signin = false;
  ASSERT_NO_FATAL_FAILURE(CheckIsCertificateAvailable(
      cert_file_path, &cert_available_before_signin));
  EXPECT_TRUE(cert_available_before_signin);

  LoginRegularUser();
  bool cert_available_after_signin = false;
  ASSERT_NO_FATAL_FAILURE(CheckIsCertificateAvailable(
      cert_file_path, &cert_available_after_signin));
  EXPECT_FALSE(cert_available_after_signin);
}

// Self-test: CheckIsCertificateAvailable shall return false for a certificate
// we have not made available through UntrustedAuthoritiesCache.
TEST_F(UntrustedAuthoritiesCacheTest, OtherCertNotAvailable) {
  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});
  base::FilePath other_cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_2_ca.pem"));
  bool cert_available = false;
  ASSERT_NO_FATAL_FAILURE(
      CheckIsCertificateAvailable(other_cert_file_path, &cert_available));
  EXPECT_FALSE(cert_available);
}

// Check that certificates incoming after a user has signed in are not made
// available.
TEST_F(UntrustedAuthoritiesCacheTest, CetNotAvailableAfterUserSignin) {
  LoginRegularUser();

  base::FilePath cert_file_path =
      net::GetTestCertsDirectory().Append(FILE_PATH_LITERAL("client_1_ca.pem"));
  UpdateUntrustedAuthoritiesFromFiles({cert_file_path});
  bool cert_available = false;
  ASSERT_NO_FATAL_FAILURE(
      CheckIsCertificateAvailable(cert_file_path, &cert_available));
  EXPECT_FALSE(cert_available);
}

}  // namespace
}  // namespace policy
