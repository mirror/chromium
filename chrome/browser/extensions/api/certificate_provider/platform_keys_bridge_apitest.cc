// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys_service.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys_service_factory.h"
#include "chrome/browser/extensions/api/certificate_provider/certificate_provider_test_util.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_utils.h"
#include "crypto/rsa_private_key.h"
#include "extensions/common/extension.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Return;
using testing::_;

namespace chromeos {

namespace {

void IgnoreResult(const base::Closure& callback, const base::Value* value) {
  callback.Run();
}

void StoreString(std::string* result,
                 const base::Closure& callback,
                 const base::Value* value) {
  value->GetAsString(result);
  callback.Run();
}

void StoreDigest(std::vector<uint8_t>* digest,
                 const base::Closure& callback,
                 const base::Value* value) {
  ASSERT_TRUE(value->is_blob()) << "Unexpected value in StoreDigest";
  digest->assign(value->GetBlob().begin(), value->GetBlob().end());
  callback.Run();
}

// These test cases provide support for loading two extensions:
// CertProvider: An extension which provides certificates using the
// certificateProvider API (loaded in a background tab). Currently, the test
// cases re-use the certificate_provider extension used by the
// certificate_provider_apitest. Consumer: An extension which uses the
// platformKeys API to access certificates provided by the CertProvider
// extension. This CertProvider/Consumer naming schame is reflected in the
// function names.
class CertificateProviderPlatformKeysApiTest : public ExtensionApiTest {
 public:
  CertificateProviderPlatformKeysApiTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
  }

 protected:
  void TearDownOnMainThread() override {
    consumer_test_message_listener_.reset();
    cert_provider_result_catcher_.reset();
    ExtensionApiTest::TearDownOnMainThread();
  }

  chromeos::PlatformKeysService* GetPlatformKeysService() {
    return chromeos::PlatformKeysServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  void CertProviderExpectPositiveResult() {
    EXPECT_TRUE(cert_provider_result_catcher_->GetNextResult())
        << cert_provider_result_catcher_->message();
  }

  // Sets up the certificate_provider test extension in a new background tab.
  // Returns the WebContents of that tab.
  content::WebContents* CertProviderLoadInBackgroundTab() {
    CHECK(!cert_provider_result_catcher_);
    cert_provider_result_catcher_ =
        base::MakeUnique<extensions::ResultCatcher>();

    const base::FilePath cert_provider_extension_path =
        test_data_dir_.AppendASCII("certificate_provider/test_extension");
    const extensions::Extension* const cert_provider_extension =
        LoadExtension(cert_provider_extension_path);

    auto cert_provider_url =
        cert_provider_extension->GetResourceURL("basic.html");
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), cert_provider_url, WindowOpenDisposition::NEW_BACKGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    TabStripModel* tab_strip = browser()->tab_strip_model();
    content::WebContents* const cert_provider_extension_contents =
        tab_strip->GetWebContentsAt(tab_strip->active_index() + 1);
    CHECK_EQ(cert_provider_extension_contents->GetVisibleURL(),
             cert_provider_url);

    // Wait for certificate_provider extension to load and register.
    CertProviderExpectPositiveResult();
    VLOG(1) << "certificate_provider extension registered.";
    return cert_provider_extension_contents;
  }

  void CertProviderWaitCertRequest() {
    VLOG(1) << "Wait for certificate_provider extension to receive the cert "
               "request.";
    CertProviderExpectPositiveResult();
  }

  void CertProviderWaitSignRequest() {
    VLOG(1) << "Wait for certificate_provider extension to receive the sign "
               "request.";
    CertProviderExpectPositiveResult();
  }

  void CertProviderDoSign(base::FilePath key_pk8_path,
                          content::WebContents* const extension_contents,
                          bool success) {
    VLOG(1) << "Fetch the digest from the sign request.";
    std::vector<uint8_t> request_digest;
    {
      base::RunLoop run_loop;
      extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
          base::ASCIIToUTF16("signDigestRequest.digest;"),
          base::Bind(&StoreDigest, &request_digest, run_loop.QuitClosure()));
      run_loop.Run();
    }

    // Note that the hash algorithm has been normalized in the getKeyPair()
    // function (see basic.js).
    VLOG(1) << "Fetch the hash algorithm from the sign request.";
    std::string request_hash_algorithm;
    {
      base::RunLoop run_loop;
      extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
          base::ASCIIToUTF16("signDigestRequest.hash;"),
          base::Bind(&StoreString, &request_hash_algorithm,
                     run_loop.QuitClosure()));
      run_loop.Run();
    }

    VLOG(1) << "Sign the digest using the private key.";
    std::string key_pk8;
    base::ReadFileToString(key_pk8_path, &key_pk8);

    const uint8_t* const key_pk8_begin =
        reinterpret_cast<const uint8_t*>(key_pk8.data());
    std::unique_ptr<crypto::RSAPrivateKey> key(
        crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(std::vector<uint8_t>(
            key_pk8_begin, key_pk8_begin + key_pk8.size())));
    ASSERT_TRUE(key);

    std::vector<uint8_t> signature;
    EXPECT_TRUE(platform_keys_test_util::RsaSign(
        request_digest, request_hash_algorithm, key.get(), &signature));

    std::string replyJsCode;
    if (success) {
      VLOG(1) << "Inject the signature back to the extension and let it reply.";
      replyJsCode = "replyWithSignature(" +
                    platform_keys_test_util::JsUint8Array(signature) + ");";
    } else {
      VLOG(1) << "Let the extension report an error.";
      replyJsCode = "replyWithError();";
    }

    {
      base::RunLoop run_loop;
      extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
          base::ASCIIToUTF16(replyJsCode),
          base::Bind(&IgnoreResult, run_loop.QuitClosure()));
      run_loop.Run();
    }
  }

  void ConsumerExpectMessage(const std::string& expected_message) {
    consumer_test_message_listener_->WaitUntilSatisfied();

    ASSERT_EQ(consumer_test_message_listener_->message(), expected_message);

    consumer_test_message_listener_->Reply(std::string());
    consumer_test_message_listener_->Reset();
  }

  // Wait for certificate_provider extension to respond to certificates request.
  // |test_case_name| is passed to the extension's basic.html page as 'search'
  // so it can select the test suite (function) to run. |test_case_parameter| is
  // passed as 'hash' so we can have parametrized test cases.
  void ConsumerLoadInCurrentTab(const std::string& test_case_name,
                                const std::string& test_case_parameter) {
    VLOG(1) << "Load platform_keys_bridge_test_extension.";

    const base::FilePath platform_keys_bridge_extension_path =
        test_data_dir_.AppendASCII(
            "certificate_provider/platform_keys_bridge_test_extension");
    const extensions::Extension* const platform_keys_bridge_extension =
        LoadExtension(platform_keys_bridge_extension_path);

    CHECK(!consumer_test_message_listener_);
    consumer_test_message_listener_ =
        base::MakeUnique<ExtensionTestMessageListener>(true /* will_reply */);
    consumer_test_message_listener_->set_extension_id(
        platform_keys_bridge_extension->id());

    ui_test_utils::NavigateToURL(
        browser(),
        platform_keys_bridge_extension->GetResourceURL(
            "basic.html?" + test_case_name + "#" + test_case_parameter));

    VLOG(1) << "platform_keys_bridge_test_extension loaded.";
  }

  void ConsumerWaitSelectFinished() {
    ConsumerExpectMessage("Matched expected certificate");
  }

  void ConsumerWaitGetKeyPairFinished() {
    ConsumerExpectMessage("Got keypair");
  }

  void ConsumerWaitGetKeyPairError() {
    ConsumerExpectMessage("getKeyPair error");
  }

  void ConsumerWaitSignatureAccepted() {
    ConsumerExpectMessage("Accepted signature");
  }

  void ConsumerWaitErrorOnSign() {
    ConsumerExpectMessage("Sign operation failed");
  }

  // Listens for results from the cert_provider test extension.
  // Note that this listens to results from all extensions technically, but only
  // the cert_provider test extension sends this kind of results.
  std::unique_ptr<extensions::ResultCatcher> cert_provider_result_catcher_;

  // Listens for messages (sent through chrome.test.sendMessage) from the
  // platform_keys_bridge_test_extension.
  std::unique_ptr<ExtensionTestMessageListener> consumer_test_message_listener_;

  policy::MockConfigurationPolicyProvider provider_;
};

// Same as CertificateProviderPlatformKeysApiTest, but allows for parametrized
// test case instantiations, to test signing with several hash function.
class CertificateProviderPlatformKeysApiTestWithHash
    : public CertificateProviderPlatformKeysApiTest,
      public testing::WithParamInterface<const char*> {
 public:
  CertificateProviderPlatformKeysApiTestWithHash() {}
  ~CertificateProviderPlatformKeysApiTestWithHash() {}

 protected:
  std::string selected_hash_function() const { return GetParam(); }
};

// A delegate which will expect exactly one certificate to be available and
// select that certificate. This translates to the user selecting the
// certificate on a certificate selection dialog in a non-test environment.
class TestSelectOnlyCertDelegate
    : public chromeos::PlatformKeysService::SelectDelegate {
 public:
  TestSelectOnlyCertDelegate() {}
  ~TestSelectOnlyCertDelegate() override {}

  void Select(const std::string& extension_id,
              const net::CertificateList& certs,
              const CertificateSelectedCallback& callback,
              content::WebContents* web_contents,
              content::BrowserContext* context) override {
    ASSERT_TRUE(web_contents);
    ASSERT_TRUE(context);
    ASSERT_EQ(1U, certs.size())
        << "Expected exactly 1 certificate to select from";
    scoped_refptr<net::X509Certificate> selection;
    selection = certs[0];
    callback.Run(selection);
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_P(CertificateProviderPlatformKeysApiTestWithHash, Basic) {
  VLOG(0) << "Running signing test with hash algorithm "
          << selected_hash_function();

  // Setup a cert selection delegate
  GetPlatformKeysService()->SetSelectDelegate(
      base::MakeUnique<TestSelectOnlyCertDelegate>());

  // Load the certificate_provider extension in a background tab.
  content::WebContents* cert_provider_extension_contents =
      CertProviderLoadInBackgroundTab();

  // Now load the platform_keys_bridge test extension and select the test case.
  ConsumerLoadInCurrentTab("basic", selected_hash_function());

  // Wait for certificate_provider extension to respond to certificates request.
  CertProviderWaitCertRequest();

  // Wait for platform_keys_bridge test extension to receive the matched
  // certificate info.
  ConsumerWaitSelectFinished();

  // Wait for platform_keys_bridge test extension to receive the keypair.
  ConsumerWaitGetKeyPairFinished();

  // Wait for the certificate_provider extension to receive the sign request.
  CertProviderWaitSignRequest();

  CertProviderDoSign(
      test_data_dir_.AppendASCII("certificate_provider/root2_l1_leaf.pk8"),
      cert_provider_extension_contents, true /* success */);

  ConsumerWaitSignatureAccepted();
}

// Note that we are intentionally passing the hash algorithm names in mixed
// case. The platformKeys.getKeyPair() API call is supposed to normalize the
// hash function name according to WebCrypto algorithm normalization rules, so
// case does not matter here.
// Additionally, the js counterpart of this test case selects the pre-generated
// signature file to compare against case insensitively.
INSTANTIATE_TEST_CASE_P(
    CertificateProviderPlatformKeysApiTestWithHash,
    CertificateProviderPlatformKeysApiTestWithHash,
    testing::Values("Sha-1", "Sha-256", "Sha-384", "Sha-512"));

// Verifies that an error is propagated to subtleCrypto.sign() if the
// certificateProvider extension reports an error in its onSignDigestRequested
// handler.
IN_PROC_BROWSER_TEST_F(CertificateProviderPlatformKeysApiTest, SignFailed) {
  // Setup a cert selection delegate
  GetPlatformKeysService()->SetSelectDelegate(
      base::MakeUnique<TestSelectOnlyCertDelegate>());

  // Load the certificate_provider extension in a background tab.
  content::WebContents* cert_provider_extension_contents =
      CertProviderLoadInBackgroundTab();

  // Now load the platform_keys_bridge test extension and select the test case.
  ConsumerLoadInCurrentTab("basic", "Sha-256");

  // Wait for certificate_provider extension to respond to certificates request.
  CertProviderWaitCertRequest();

  // Wait for platform_keys_bridge test extension to receive the matched
  // certificate info.
  ConsumerWaitSelectFinished();

  // Wait for platform_keys_bridge test extension to receive the keypair.
  ConsumerWaitGetKeyPairFinished();

  // Wait for the certificate_provider extension to receive the sign request.
  CertProviderWaitSignRequest();

  // Report an error from signing in the certificate_provider extension
  CertProviderDoSign(
      test_data_dir_.AppendASCII("certificate_provider/root2_l1_leaf.pk8"),
      cert_provider_extension_contents, false /* success */);

  ConsumerWaitErrorOnSign();
}

// Verifies that even if the certificate supports MD5_SHA1 hashing according to
// supportedHashes passed on onCertificatesRequested, signing with hash "none"
// is not allowed through platformKeys.
IN_PROC_BROWSER_TEST_F(CertificateProviderPlatformKeysApiTest,
                       HashNoneNotSupported) {
  // Setup a cert selection delegate
  GetPlatformKeysService()->SetSelectDelegate(
      base::MakeUnique<TestSelectOnlyCertDelegate>());

  // Load the certificate_provider extension in a background tab.
  CertProviderLoadInBackgroundTab();

  // Now load the platform_keys_bridge test extension and select the test case.
  ConsumerLoadInCurrentTab("hashNoneNotSupported",
                           std::string() /* test_case_parameter */);

  // Wait for certificate_provider extension to respond to certificates request.
  CertProviderWaitCertRequest();

  // Wait for platform_keys_bridge test extension to receive the matched
  // certificate info.
  ConsumerWaitSelectFinished();

  // Wait for platform_keys_bridge test extension to receive an error about the
  // unsupported hash.
  ConsumerWaitGetKeyPairError();
}

// Verify that a hash algorithm which is supported by platformKeys and
// certificateProvider APIs (SHA-256), but not supported by the specific
// certificate according to supportedHashes passed on onCertificatesRequested,
// results in a "Hash not supported." error.
IN_PROC_BROWSER_TEST_F(CertificateProviderPlatformKeysApiTest, WrongHash) {
  // Setup a cert selection delegate
  GetPlatformKeysService()->SetSelectDelegate(
      base::MakeUnique<TestSelectOnlyCertDelegate>());

  // Load the certificate_provider extension in a background tab.
  CertProviderLoadInBackgroundTab();

  // Now load the platform_keys_bridge test extension and select the test case.
  ConsumerLoadInCurrentTab("wrongHash",
                           std::string() /* test_case_parameter */);

  // Wait for certificate_provider extension to respond to certificates request.
  CertProviderWaitCertRequest();

  // Wait for platform_keys_bridge test extension to receive the matched
  // certificate info.
  ConsumerWaitSelectFinished();

  // Wait for platform_keys_bridge test extension to receive an error about the
  // unsupported hash.
  ConsumerWaitGetKeyPairError();
}

// Verify that trying to sign data using a certificate not provided by either
// the platform or a certificateProvider API fails with "Key not found".
IN_PROC_BROWSER_TEST_F(CertificateProviderPlatformKeysApiTest, InvalidCert) {
  // Load the certificate_provider extension in a background tab.
  CertProviderLoadInBackgroundTab();

  // Now load the platform_keys_bridge test extension and select the test case.
  ConsumerLoadInCurrentTab("invalidCert",
                           std::string() /* test_case_parameter */);

  // Wait for platform_keys_bridge test extension to receive an error about the
  // missing cert.
  ConsumerWaitGetKeyPairError();
}

}  // namespace chromeos
