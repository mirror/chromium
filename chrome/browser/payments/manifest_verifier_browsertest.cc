// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/manifest_verifier.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/web_data_service_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/payments/content/payment_manifest_parser_host.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/core/payment_manifest_downloader.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace payments {
namespace {

// Downloads everything from the test server.
class TestDownloader : public PaymentMethodManifestDownloaderInterface {
 public:
  explicit TestDownloader(
      const scoped_refptr<net::URLRequestContextGetter>& context)
      : impl_(context) {}

  ~TestDownloader() override {}

  // PaymentMethodManifestDownloaderInterface implementation.
  void DownloadPaymentMethodManifest(
      const GURL& url,
      PaymentManifestDownloadCallback callback) override {
    // The length of "https://" URL prefix.
    static const size_t kHttpsPrefixLength = 8;
    impl_.DownloadPaymentMethodManifest(
        GURL(test_server_url_.spec() + url.spec().substr(kHttpsPrefixLength)),
        std::move(callback));
  }

  void set_test_server_url(const GURL& url) { test_server_url_ = url; }

 private:
  // The actual downloader.
  PaymentManifestDownloader impl_;

  // The base URL of the test server, e.g., "https://127.0.0.1:8080/".
  GURL test_server_url_;

  DISALLOW_COPY_AND_ASSIGN(TestDownloader);
};

// Tests for the manifest verifier.
class ManifestVerifierBrowserTest : public InProcessBrowserTest {
 public:
  ManifestVerifierBrowserTest() {}
  ~ManifestVerifierBrowserTest() override {}

  // Starts the HTTPS test server on localhost.
  void SetUpOnMainThread() override {
    https_server_.reset(
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS));
    ASSERT_TRUE(https_server_->InitializeAndListen());
    https_server_->ServeFilesFromSourceDirectory(
        "components/test/data/payments");
    https_server_->StartAcceptingConnections();
  }

  // Runs the verifier on the |apps| and blocks until the verification has
  // completed.
  void Verify(content::PaymentAppProvider::PaymentApps apps) {
    auto downloader = base::MakeUnique<TestDownloader>(
        content::BrowserContext::GetDefaultStoragePartition(
            browser()
                ->tab_strip_model()
                ->GetActiveWebContents()
                ->GetBrowserContext())
            ->GetURLRequestContext());
    downloader->set_test_server_url(https_server_->GetURL("/"));

    // The verifier object deletes itself after invoking the callback.
    payments::ManifestVerifier* verifier = new payments::ManifestVerifier(
        std::move(downloader),
        base::MakeUnique<payments::PaymentManifestParserHost>(),
        WebDataServiceFactory::GetPaymentManifestWebDataForProfile(
            ProfileManager::GetActiveUserProfile(),
            ServiceAccessType::EXPLICIT_ACCESS));
    verifier->StartUtilityProcess();

    base::RunLoop run_loop;
    payment_apps_verified_closure_ = run_loop.QuitClosure();

    verifier->Verify(
        std::move(apps),
        base::BindOnce(&ManifestVerifierBrowserTest::OnPaymentAppsVerified,
                       base::Unretained(this)));

    run_loop.Run();
  }

  // Returns the apps that have been verified by the Verify() method.
  const content::PaymentAppProvider::PaymentApps& verified_apps() const {
    return verified_apps_;
  }

  // Expects that the verified payment app with |id| has the |expected_scope|
  // and the |expected_methods|.
  void ExpectApp(int64_t id,
                 const std::string& expected_scope,
                 const std::set<std::string>& expected_methods,
                 bool is_repeat) {
    ASSERT_NE(verified_apps().end(), verified_apps().find(id))
        << "One of the verified apps should have registration id " << id
        << (is_repeat ? " again." : ".");
    EXPECT_EQ(GURL(expected_scope), verified_apps().find(id)->second->scope)
        << "The app with registration id " << id << " should have "
        << expected_scope << " scope" << (is_repeat ? " again." : ".");
    std::set<std::string> actual_methods(
        verified_apps().find(id)->second->enabled_methods.begin(),
        verified_apps().find(id)->second->enabled_methods.end());
    EXPECT_EQ(expected_methods, actual_methods)
        << "The " << expected_scope << " app should have " << expected_scope
        << *expected_methods.begin()
        << (expected_methods.size() > 1 ? " and " : "")
        << (expected_methods.size() > 1 ? *(++expected_methods.begin()) : "")
        << " method" << (expected_methods.size() > 1 ? "s" : "")
        << (is_repeat ? " again." : ".");
  }

 private:
  // Called by the verifier upon completed verification. These |apps| have only
  // valid payment methods.
  void OnPaymentAppsVerified(content::PaymentAppProvider::PaymentApps apps) {
    verified_apps_ = std::move(apps);
    payment_apps_verified_closure_.Run();
  }

  // Serves the payment method manifest files.
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  // The apps that have been verified by the Verify() method.
  content::PaymentAppProvider::PaymentApps verified_apps_;

  // Used to stop blocking the Verify() method and continue the test.
  base::Closure payment_apps_verified_closure_;

  DISALLOW_COPY_AND_ASSIGN(ManifestVerifierBrowserTest);
};

// Absence of payment handlers should result in absence of verified payment
// handlers.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest, NoApps) {
  {
    Verify(content::PaymentAppProvider::PaymentApps());

    EXPECT_TRUE(verified_apps().empty());
  }

  // Repeat verifications should have identical results.
  {
    Verify(content::PaymentAppProvider::PaymentApps());

    EXPECT_TRUE(verified_apps().empty());
  }
}

// A payment handler without any payment method names is not valid.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest, NoMethods) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");

    Verify(std::move(apps));

    EXPECT_TRUE(verified_apps().empty());
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");

    Verify(std::move(apps));

    EXPECT_TRUE(verified_apps().empty());
  }
}

// A payment handler with an unknown non-URL payment method name is not valid.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       UnknownPaymentMethodNameIsRemoved) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("unknown");

    Verify(std::move(apps));

    EXPECT_TRUE(verified_apps().empty());
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("unknown");

    Verify(std::move(apps));

    EXPECT_TRUE(verified_apps().empty());
  }
}

// A payment handler with "basic-card" payment method name is valid.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest, KnownPaymentMethodName) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("basic-card");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"basic-card"}, false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("basic-card");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"basic-card"}, true);
  }
}

// A payment handler with both "basic-card" and "interledger" payment method
// names is valid.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       TwoKnownPaymentMethodNames) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("basic-card");
    apps[0]->enabled_methods.push_back("interledger");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"basic-card", "interledger"},
              false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("basic-card");
    apps[0]->enabled_methods.push_back("interledger");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"basic-card", "interledger"},
              true);
  }
}

// Two payment handlers with "basic-card" payment method names are both valid.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       TwoAppsWithKnownPaymentMethodNames) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("basic-card");
    apps[1] = base::MakeUnique<content::StoredPaymentApp>();
    apps[1]->scope = GURL("https://alicepay.com/webpay");
    apps[1]->enabled_methods.push_back("basic-card");

    Verify(std::move(apps));

    EXPECT_EQ(2U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"basic-card"}, false);
    ExpectApp(1, "https://alicepay.com/webpay", {"basic-card"}, false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("basic-card");
    apps[1] = base::MakeUnique<content::StoredPaymentApp>();
    apps[1]->scope = GURL("https://alicepay.com/webpay");
    apps[1]->enabled_methods.push_back("basic-card");
    Verify(std::move(apps));

    EXPECT_EQ(2U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"basic-card"}, true);
    ExpectApp(1, "https://alicepay.com/webpay", {"basic-card"}, true);
  }
}

// Verify that a payment handler from https://bobpay.com/webpay can use the
// payment method name https://frankpay.com/webpay, because
// https://frankpay.com/payment-manifest.json contains "supported_origins": "*".
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       BobPayHandlerCanUseMethodThatSupportsAllOrigins) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("https://frankpay.com/webpay");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"https://frankpay.com/webpay"},
              false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("https://frankpay.com/webpay");
    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/webpay", {"https://frankpay.com/webpay"},
              true);
  }
}

// Verify that a payment handler from an unreachable website can use the payment
// method name https://frankpay.com/webpay, because
// https://frankpay.com/payment-manifest.json contains "supported_origins": "*".
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       Handler404CanUseMethodThatSupportsAllOrigins) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://404.com/webpay");
    apps[0]->enabled_methods.push_back("https://frankpay.com/webpay");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://404.com/webpay", {"https://frankpay.com/webpay"},
              false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://404.com/webpay");
    apps[0]->enabled_methods.push_back("https://frankpay.com/webpay");
    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://404.com/webpay", {"https://frankpay.com/webpay"},
              true);
  }
}

// Verify that a payment handler from anywhere on https://bobpay.com can use the
// payment method name from anywhere else on https://bobpay.com, because of the
// origin match.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       BobPayCanUseAnyMethodOnOwnOrigin) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/anything/here");
    apps[0]->enabled_methods.push_back(
        "https://bobpay.com/does/not/matter/whats/here");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/anything/here",
              {"https://bobpay.com/does/not/matter/whats/here"}, false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/anything/here");
    apps[0]->enabled_methods.push_back(
        "https://bobpay.com/does/not/matter/whats/here");
    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://bobpay.com/anything/here",
              {"https://bobpay.com/does/not/matter/whats/here"}, true);
  }
}

// Verify that a payment handler from anywhere on an unreachable website can use
// the payment method name from anywhere else on the same unreachable website,
// because they have identical origin.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest,
                       Handler404CanUseAnyMethodOnOwnOrigin) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://404.com/anything/here");
    apps[0]->enabled_methods.push_back(
        "https://404.com/does/not/matter/whats/here");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://404.com/anything/here",
              {"https://404.com/does/not/matter/whats/here"}, false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://404.com/anything/here");
    apps[0]->enabled_methods.push_back(
        "https://404.com/does/not/matter/whats/here");
    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://404.com/anything/here",
              {"https://404.com/does/not/matter/whats/here"}, true);
  }
}

// Verify that only the payment handler from https://alicepay.com/webpay can use
// payment methods https://georgepay.com/webpay and https://ikepay.com/webpay,
// because both https://georgepay.com/payment-manifest.json and
// https://ikepay.com/payment-manifest.json contain "supported_origins":
// ["https://alicepay.com"]. The payment handler from https://bobpay.com/webpay
// cannot use these payment methods, however.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest, OneSupportedOrigin) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://alicepay.com/webpay");
    apps[0]->enabled_methods.push_back("https://georgepay.com/webpay");
    apps[0]->enabled_methods.push_back("https://ikepay.com/webpay");
    apps[1] = base::MakeUnique<content::StoredPaymentApp>();
    apps[1]->scope = GURL("https://bobpay.com/webpay");
    apps[1]->enabled_methods.push_back("https://georgepay.com/webpay");
    apps[1]->enabled_methods.push_back("https://ikepay.com/webpay");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://alicepay.com/webpay",
              {"https://georgepay.com/webpay", "https://ikepay.com/webpay"},
              false);
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://alicepay.com/webpay");
    apps[0]->enabled_methods.push_back("https://georgepay.com/webpay");
    apps[0]->enabled_methods.push_back("https://ikepay.com/webpay");
    apps[1] = base::MakeUnique<content::StoredPaymentApp>();
    apps[1]->scope = GURL("https://bobpay.com/webpay");
    apps[1]->enabled_methods.push_back("https://georgepay.com/webpay");
    apps[1]->enabled_methods.push_back("https://ikepay.com/webpay");

    Verify(std::move(apps));

    EXPECT_EQ(1U, verified_apps().size());
    ExpectApp(0, "https://alicepay.com/webpay",
              {"https://georgepay.com/webpay", "https://ikepay.com/webpay"},
              true);
  }
}

// Verify that a payment handler from https://bobpay.com/webpay cannot use
// payment method names that are unreachable websites, the origin of which does
// not match that of the payment handler.
IN_PROC_BROWSER_TEST_F(ManifestVerifierBrowserTest, PaymentMethodName404) {
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("https://404.com/webpay");
    apps[0]->enabled_methods.push_back("https://404aswell.com/webpay");

    Verify(std::move(apps));

    EXPECT_TRUE(verified_apps().empty());
  }

  // Repeat verifications should have identical results.
  {
    content::PaymentAppProvider::PaymentApps apps;
    apps[0] = base::MakeUnique<content::StoredPaymentApp>();
    apps[0]->scope = GURL("https://bobpay.com/webpay");
    apps[0]->enabled_methods.push_back("https://404.com/webpay");
    apps[0]->enabled_methods.push_back("https://404aswell.com/webpay");

    Verify(std::move(apps));

    EXPECT_TRUE(verified_apps().empty());
  }
}

}  // namespace
}  // namespace payments
