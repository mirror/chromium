// Copyright 2015 The Chromium Authors. All rights reserved.
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
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/certificate_provider/certificate_provider_service.h"
#include "chrome/browser/chromeos/certificate_provider/certificate_provider_service_factory.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys_service.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys_service_factory.h"
#include "chrome/browser/extensions/api/certificate_provider/certificate_provider_api.h"
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
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "crypto/rsa_private_key.h"
#include "extensions/common/extension.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

using testing::Return;
using testing::_;

namespace {

void IgnoreResult(const base::Closure& callback, const base::Value* value) {
  callback.Run();
}

void StoreDigest(std::vector<uint8_t>* digest,
                 const base::Closure& callback,
                 const base::Value* value) {
  ASSERT_TRUE(value->is_blob()) << "Unexpected value in StoreDigest";
  digest->assign(value->GetBlob().begin(), value->GetBlob().end());
  callback.Run();
}

// See net::SSLPrivateKey::SignDigest for the expected padding and DigestInfo
// prefixing.
bool RsaSign(const std::vector<uint8_t>& digest,
             crypto::RSAPrivateKey* key,
             std::vector<uint8_t>* signature) {
  RSA* rsa_key = EVP_PKEY_get0_RSA(key->key());
  if (!rsa_key)
    return false;

  uint8_t* prefixed_digest = nullptr;
  size_t prefixed_digest_len = 0;
  int is_alloced = 0;
  if (!RSA_add_pkcs1_prefix(&prefixed_digest, &prefixed_digest_len, &is_alloced,
                            NID_sha1, digest.data(), digest.size())) {
    return false;
  }
  size_t len = 0;
  signature->resize(RSA_size(rsa_key));
  const int rv =
      RSA_sign_raw(rsa_key, &len, signature->data(), signature->size(),
                   prefixed_digest, prefixed_digest_len, RSA_PKCS1_PADDING);
  if (is_alloced)
    free(prefixed_digest);

  if (rv) {
    signature->resize(len);
    return true;
  } else {
    signature->clear();
    return false;
  }
}

// Create a string that if evaluated in JavaScript returns a Uint8Array with
// |bytes| as content.
std::string JsUint8Array(const std::vector<uint8_t>& bytes) {
  std::string res = "new Uint8Array([";
  for (const uint8_t byte : bytes) {
    res += base::UintToString(byte);
    res += ", ";
  }
  res += "])";
  return res;
}

// These test cases provide support for loading two extensions:
// cert_provider: An extension certificate_provider extension (usually loaded in
// a background tab)
class CertificateProviderPlatformKeysApiTest : public ExtensionApiTest {
 public:
  CertificateProviderPlatformKeysApiTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override { ExtensionApiTest::SetUpOnMainThread(); }

 protected:
  chromeos::PlatformKeysService* GetPlatformKeysService() {
    return chromeos::PlatformKeysServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  void CertProviderExpectPositiveResult() {
    EXPECT_TRUE(cert_provider_result_catcher_->GetNextResult())
        << cert_provider_result_catcher_->message();
  }

  void TearDownOnMainThread() override {
    cert_provider_result_catcher_.reset();
    ExtensionApiTest::TearDownOnMainThread();
  }

  // Sets up the certificate_provider test extension in a new background tab.
  // Returns the WebContents of that tab.
  content::WebContents* CertProviderLoadInBackgroundTab(
      base::FilePath cert_provider_extension_path) {
    cert_provider_result_catcher_ =
        base::MakeUnique<extensions::ResultCatcher>();

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
    CHECK(cert_provider_extension_contents->GetVisibleURL() ==
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

  void CertProviderDoSign(base::FilePath extension_path,
                          content::WebContents* const extension_contents) {
    VLOG(1) << "Fetch the digest from the sign request.";
    std::vector<uint8_t> request_digest;
    {
      base::RunLoop run_loop;
      extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
          base::ASCIIToUTF16("signDigestRequest.digest;"),
          base::Bind(&StoreDigest, &request_digest, run_loop.QuitClosure()));
      run_loop.Run();
    }

    VLOG(1) << "Sign the digest using the private key.";
    std::string key_pk8;
    base::ReadFileToString(extension_path.AppendASCII("l1_leaf.pk8"), &key_pk8);

    const uint8_t* const key_pk8_begin =
        reinterpret_cast<const uint8_t*>(key_pk8.data());
    std::unique_ptr<crypto::RSAPrivateKey> key(
        crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(std::vector<uint8_t>(
            key_pk8_begin, key_pk8_begin + key_pk8.size())));
    ASSERT_TRUE(key);

    std::vector<uint8_t> signature;
    EXPECT_TRUE(RsaSign(request_digest, key.get(), &signature));

    VLOG(1) << "Inject the signature back to the extension and let it reply.";
    {
      base::RunLoop run_loop;
      const std::string code =
          "replyWithSignature(" + JsUint8Array(signature) + ");";
      extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
          base::ASCIIToUTF16(code),
          base::Bind(&IgnoreResult, run_loop.QuitClosure()));
      run_loop.Run();
    }
  }

  void ConsumerExpectMessage(std::string expected_message) {
    CHECK(!consumer_extension_id_.empty());

    ExtensionTestMessageListener listener(false /*will_reply*/);
    listener.set_extension_id(consumer_extension_id_);
    listener.WaitUntilSatisfied();
    ASSERT_EQ(listener.message(), expected_message);
  }

  // Wait for certificate_provider extension to respond to certificates request.
  // |test_case_name| is passed to the extension's basic.html page so it can
  // select the test suite (function) to run.
  void ConsumerLoadInCurrentTab(std::string test_case_name) {
    VLOG(1) << "Load platform_keys_bridged extension.";

    const base::FilePath platform_keys_bridged_extension_path =
        test_data_dir_.AppendASCII(
            "certificate_provider/platform_keys_bridged");
    const extensions::Extension* const platform_keys_bridged_extension =
        LoadExtension(platform_keys_bridged_extension_path);
    ui_test_utils::NavigateToURL(
        browser(), platform_keys_bridged_extension->GetResourceURL(
                       "basic.html#" + test_case_name));

    consumer_extension_id_ = platform_keys_bridged_extension->id();

    VLOG(1) << "platform_keys_bridged extension loaded.";
  }

  void ConsumerWaitSelectFinished() {
    ConsumerExpectMessage("Matched expected certificate");
  }

  void ConsumerWaitGetKeyPairFinished() {
    ConsumerExpectMessage("Got keypair");
  }

  void ConsumerWaitGetKeyPairUnsupportedHash() {
    ConsumerExpectMessage("getKeyPair error");
  }

  void ConsumerWaitSignatureAccepted() {
    ConsumerExpectMessage("Accepted signature");
  }

  // Listens for results from the cert_provider test extension.
  // Note that this listens to results from all extensions technically, but only
  // the cert_provider test extension sends this kind of results.
  std::unique_ptr<extensions::ResultCatcher> cert_provider_result_catcher_;

  // Extension id of the consumer (=platform_keys_bridged) extension.
  std::string consumer_extension_id_;

  policy::MockConfigurationPolicyProvider provider_;
};

class TestSelectDelegate
    : public chromeos::PlatformKeysService::SelectDelegate {
 public:
  TestSelectDelegate() {}
  ~TestSelectDelegate() override {}

  void Select(const std::string& extension_id,
              const net::CertificateList& certs,
              const CertificateSelectedCallback& callback,
              content::WebContents* web_contents,
              content::BrowserContext* context) override {
    ASSERT_TRUE(web_contents);
    ASSERT_TRUE(context);
    scoped_refptr<net::X509Certificate> selection;
    selection = certs[0];
    callback.Run(selection);
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(CertificateProviderPlatformKeysApiTest, Basic) {
  // Setup a cert selection delegate
  GetPlatformKeysService()->SetSelectDelegate(
      base::MakeUnique<TestSelectDelegate>());

  // Load the certificate_provider extension in a background tab.
  const base::FilePath cert_provider_extension_path =
      test_data_dir_.AppendASCII("certificate_provider");
  content::WebContents* cert_provider_extension_contents =
      CertProviderLoadInBackgroundTab(cert_provider_extension_path);

  // Now load the platform_keys_bridged extension.
  ConsumerLoadInCurrentTab("basic");

  // Wait for certificate_provider extension to respond to certificates request.
  CertProviderWaitCertRequest();

  // Wait for platform_keys_bridged extension to receive the matched certificate
  // info.
  ConsumerWaitSelectFinished();

  // Wait for platform_keys_bridged extension to receive the keypair.
  ConsumerWaitGetKeyPairFinished();

  // Wait for the certificate_provider extension to receive the sign request.
  CertProviderWaitSignRequest();

  CertProviderDoSign(cert_provider_extension_path,
                     cert_provider_extension_contents);

  ConsumerWaitSignatureAccepted();
}

IN_PROC_BROWSER_TEST_F(CertificateProviderPlatformKeysApiTest, WrongHash) {
  // Setup a cert selection delegate
  GetPlatformKeysService()->SetSelectDelegate(
      base::MakeUnique<TestSelectDelegate>());

  // Load the certificate_provider extension in a background tab.
  const base::FilePath cert_provider_extension_path =
      test_data_dir_.AppendASCII("certificate_provider");
  CertProviderLoadInBackgroundTab(cert_provider_extension_path);

  // Now load the platform_keys_bridged extension.
  ConsumerLoadInCurrentTab("wrongHash");

  // Wait for certificate_provider extension to respond to certificates request.
  CertProviderWaitCertRequest();

  // Wait for platform_keys_bridged extension to receive the matched certificate
  // info.
  ConsumerWaitSelectFinished();

  // Wait for platform_keys_bridged extension to receive the keypair.
  ConsumerWaitGetKeyPairUnsupportedHash();
}
