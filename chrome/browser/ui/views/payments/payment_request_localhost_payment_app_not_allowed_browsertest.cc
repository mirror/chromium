// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/payments/content/service_worker_payment_app_factory.h"
#include "components/payments/core/test_payment_manifest_downloader.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

// A test for localhost server that attempts to install a payment app and call
// payment request.
class PaymentRequestLocalhostPaymentAppNotAllowedTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestLocalhostPaymentAppNotAllowedTest()
      : localhost_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        features::kServiceWorkerPaymentApps);
  }

  // Starts the test servers and opens a test page.
  void SetUpOnMainThread() override {
    PaymentRequestBrowserTestBase::SetUpOnMainThread();
    ASSERT_TRUE(localhost_.InitializeAndListen());
    localhost_.ServeFilesFromSourceDirectory(
        "components/test/data/payments/alicepay.com");
    localhost_.StartAcceptingConnections();
  }

  net::EmbeddedTestServer localhost_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestLocalhostPaymentAppNotAllowedTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestLocalhostPaymentAppNotAllowedTest,
                       LocalhostNotallowedByDefault) {
  GURL url = localhost_.GetURL("/app1/");
  ui_test_utils::NavigateToURL(browser(), url);
  std::string method_name = url.spec();
  std::string script = "install('" + method_name + "');";
  std::string contents;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      browser()->tab_strip_model()->GetActiveWebContents(), script, &contents))
      << "Script execution failed: " << script;
  ASSERT_NE(std::string::npos,
            contents.find("Payment app for \"" + method_name +
                          "\" method installed."))
      << method_name << " method install message not found in:\n"
      << contents;
}

}  // namespace
}  // namespace payments
