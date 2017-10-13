// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_app_factory.h"

#include <algorithm>
#include <utility>

#include "base/command_line.h"
#include "base/run_loop.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/web_data_service_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/remoting/remote_test_helper.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/core/test_payment_manifest_downloader.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

// Tests for the service worker payment app factory.
class ServiceWorkerPaymentAppFactoryBrowserTest : public InProcessBrowserTest {
 public:
  ServiceWorkerPaymentAppFactoryBrowserTest() {}
  ~ServiceWorkerPaymentAppFactoryBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed to
    // load pages from the test servers with custom hostnames without an
    // interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);

    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
  }

  // Starts the test severs and opens a test page on alicepay.com.
  void SetUpOnMainThread() override {
    alicepay_ = StartTestServer("alicepay.com");
    bobpay_ = StartTestServer("bobpay.com");
    frankpay_ = StartTestServer("frankpay.com");
    georgepay_ = StartTestServer("georgepay.com");

    ASSERT_NE(nullptr, alicepay_);
    ASSERT_NE(nullptr, bobpay_);
    ASSERT_NE(nullptr, frankpay_);
    ASSERT_NE(nullptr, georgepay_);

    ui_test_utils::NavigateToURL(browser(),
                                 alicepay_->GetURL("alicepay.com", "/"));
  }

  // Invokes the JavaScript function install(|method_name|) in
  // components/test/data/payments/alicepay.com/index.js, which responds back
  // via domAutomationController.
  void InstallPaymentAppForMethod(const std::string& method_name) {
    std::string contents;
    std::string script = "install('" + method_name + "');";
    ASSERT_TRUE(content::ExecuteScriptAndExtractString(
        browser()->tab_strip_model()->GetActiveWebContents(), script,
        &contents))
        << "Script execution failed: " << script;
    ASSERT_NE(std::string::npos,
              contents.find("Payment app for \"" + method_name +
                            "\" method installed."))
        << method_name << " method install message not found in:\n"
        << contents;
  }

  // Retrieves all valid payment apps that can handle the methods in
  // |payment_method_identifiers_set|. Blocks until the factory has finished
  // using all resources.
  void GetAllPaymentAppsForMethods(
      const std::set<std::string>& payment_method_identifiers_set) {
    content::BrowserContext* context = browser()
                                           ->tab_strip_model()
                                           ->GetActiveWebContents()
                                           ->GetBrowserContext();
    auto downloader = std::make_unique<TestDownloader>(
        content::BrowserContext::GetDefaultStoragePartition(context)
            ->GetURLRequestContext());
    downloader->AddTestServerURL("https://alicepay.com/",
                                 alicepay_->GetURL("alicepay.com", "/"));
    downloader->AddTestServerURL("https://bobpay.com/",
                                 bobpay_->GetURL("bobpay.com", "/"));
    downloader->AddTestServerURL("https://frankpay.com/",
                                 frankpay_->GetURL("frankpay.com", "/"));
    downloader->AddTestServerURL("https://georgepay.com/",
                                 georgepay_->GetURL("georgepay.com", "/"));
    ServiceWorkerPaymentAppFactory factory;
    factory.IgnorePortInAppScopeForTesting();

    base::RunLoop run_loop;
    finished_using_resources_closure_ = run_loop.QuitClosure();

    factory.GetAllPaymentApps(
        context, std::move(downloader),
        WebDataServiceFactory::GetPaymentManifestWebDataForProfile(
            Profile::FromBrowserContext(context),
            ServiceAccessType::EXPLICIT_ACCESS),
        payment_method_identifiers_set,
        base::BindOnce(
            &ServiceWorkerPaymentAppFactoryBrowserTest::OnGotAllPaymentApps,
            base::Unretained(this)),
        base::BindOnce(&ServiceWorkerPaymentAppFactoryBrowserTest::
                           OnFinishedUsingResources,
                       base::Unretained(this)));

    run_loop.Run();
  }

  // Returns the apps that have been found in GetAllPaymentAppsForMethods().
  const content::PaymentAppProvider::PaymentApps& apps() const { return apps_; }

  // Expects that the first app has the |expected_method|.
  void ExpectPaymentAppWithMethod(const std::string& expected_method) {
    ASSERT_FALSE(apps().empty());
    content::StoredPaymentApp* app = apps().begin()->second.get();
    ASSERT_NE(nullptr, app);
    EXPECT_NE(app->enabled_methods.end(),
              std::find(app->enabled_methods.begin(),
                        app->enabled_methods.end(), expected_method))
        << "Unable to find payment method " << expected_method
        << " in the list of enabled methods for the app " << app->scope;
  }

 private:
  // Called by the factory upon completed app lookup. These |apps| have only
  // valid payment methods.
  void OnGotAllPaymentApps(content::PaymentAppProvider::PaymentApps apps) {
    apps_ = std::move(apps);
  }

  // Called by the factory upon saving the manifests in cache.
  void OnFinishedUsingResources() { finished_using_resources_closure_.Run(); }

  // Starts a test server for |hostname|.
  std::unique_ptr<net::EmbeddedTestServer> StartTestServer(
      const std::string& hostname) {
    auto test_server = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    host_resolver()->AddRule(hostname, "127.0.0.1");
    if (!test_server->InitializeAndListen())
      return nullptr;
    test_server->ServeFilesFromSourceDirectory(
        "components/test/data/payments/" + hostname);
    test_server->StartAcceptingConnections();
    return test_server;
  }

  // https://alicepay.com hosts the payment app.
  std::unique_ptr<net::EmbeddedTestServer> alicepay_;

  // https://bobpay.com/webpay does not permit any other origin to use this
  // payment method.
  std::unique_ptr<net::EmbeddedTestServer> bobpay_;

  // https://frankpay.com/webpay supports payment apps from any origin.
  std::unique_ptr<net::EmbeddedTestServer> frankpay_;

  // https://georgepay.com/webpay supports payment apps only from
  // https://alicepay.com.
  std::unique_ptr<net::EmbeddedTestServer> georgepay_;

  // The apps that have been verified by the Verify() method.
  content::PaymentAppProvider::PaymentApps apps_;

  // Used to stop blocking the Verify() method and continue the test.
  base::Closure finished_using_resources_closure_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentAppFactoryBrowserTest);
};

// A payment app has to be installed first.
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest, NoApps) {
  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    EXPECT_TRUE(apps().empty());
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    EXPECT_TRUE(apps().empty());
  }
}

// Unknown payment method names are not permitted.
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest,
                       UnknownMethod) {
  InstallPaymentAppForMethod("unknown-payment-method");

  {
    GetAllPaymentAppsForMethods({"unknown-payment-method", "basic-card",
                                 "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    EXPECT_TRUE(apps().empty());
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"unknown-payment-method", "basic-card",
                                 "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    EXPECT_TRUE(apps().empty());
  }
}

// A payment app can use "basic-card" payment method.
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest, BasicCard) {
  InstallPaymentAppForMethod("basic-card");

  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("basic-card");
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("basic-card");
  }
}

// A payment app can use any payment method name from its own origin.
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest, OwnOrigin) {
  InstallPaymentAppForMethod("https://alicepay.com/webpay");

  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("https://alicepay.com/webpay");
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("https://alicepay.com/webpay");
  }
}

// A payment app from https://alicepay.com cannot use the payment method
// https://bobpay.com/webpay, because https://bobpay.com/payment-method.json
// does not have an entry for "supported_origins".
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest,
                       NotSupportedOrigin) {
  InstallPaymentAppForMethod("https://bobpay.com/webpay");

  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    EXPECT_TRUE(apps().empty());
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"basic-card", "https://alicepay.com/webpay",
                                 "https://bobpay.com/webpay"});

    EXPECT_TRUE(apps().empty());
  }
}

// A payment app from https://alicepay.com can use the payment method
// https://frankpay.com/webpay, because https://frankpay.com/payment-method.json
// specifies "supported_origins": "*" (all origins supported).
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest,
                       AllOringsSupported) {
  InstallPaymentAppForMethod("https://frankpay.com/webpay");

  {
    GetAllPaymentAppsForMethods({"https://frankpay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("https://frankpay.com/webpay");
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"https://frankpay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("https://frankpay.com/webpay");
  }
}

// A payment app from https://alicepay.com can use the payment method
// https://georgepay.com/webpay, because
// https://georgepay.com/payment-method.json explicitly includes
// "https://alicepay.com" as one of the "supported_origins".
IN_PROC_BROWSER_TEST_F(ServiceWorkerPaymentAppFactoryBrowserTest,
                       SupportedOrigin) {
  InstallPaymentAppForMethod("https://georgepay.com/webpay");

  {
    GetAllPaymentAppsForMethods({"https://georgepay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("https://georgepay.com/webpay");
  }

  // Repeat lookups should have identical results.
  {
    GetAllPaymentAppsForMethods({"https://georgepay.com/webpay"});

    ASSERT_EQ(1U, apps().size());
    ExpectPaymentAppWithMethod("https://georgepay.com/webpay");
  }
}

}  // namespace
}  // namespace payments
