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
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

class PaymentRequestPaymentAppTest : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestPaymentAppTest()
      : alicepay_(net::EmbeddedTestServer::TYPE_HTTPS),
        bobpay_(net::EmbeddedTestServer::TYPE_HTTPS),
        frankpay_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        features::kServiceWorkerPaymentApps);
  }

  // Starts the test severs and opens a test page on alicepay.com.
  void SetUpOnMainThread() override {
    PaymentRequestBrowserTestBase::SetUpOnMainThread();

    ASSERT_TRUE(StartTestServer("alicepay.com", &alicepay_));
    ASSERT_TRUE(StartTestServer("bobpay.com", &bobpay_));
    ASSERT_TRUE(StartTestServer("frankpay.com", &frankpay_));
  }

  // Sets a TestDownloader for alicepay.com, bobpay.com and frankpay.com to
  // ServiceWorkerPaymentAppFactory, and ignores port in app scope.
  void SetDownloaderAndIgnorePortInAppScopeForTesting() {
    content::BrowserContext* context = browser()
                                           ->tab_strip_model()
                                           ->GetActiveWebContents()
                                           ->GetBrowserContext();
    auto downloader = std::make_unique<TestDownloader>(
        content::BrowserContext::GetDefaultStoragePartition(context)
            ->GetURLRequestContext());
    downloader->AddTestServerURL("https://alicepay.com/",
                                 alicepay_.GetURL("alicepay.com", "/"));
    downloader->AddTestServerURL("https://bobpay.com/",
                                 bobpay_.GetURL("bobpay.com", "/"));
    downloader->AddTestServerURL("https://frankpay.com/",
                                 frankpay_.GetURL("frankpay.com", "/"));
    ServiceWorkerPaymentAppFactory::GetInstance()
        ->SetDownloaderAndIgnorePortInAppScopeForTesting(std::move(downloader));
  }

  GURL GetAlicePayInstallURL(const std::string& scope) {
    return alicepay_.GetURL("alicepay.com", scope);
  }

 private:
  // https://alicepay.com hosts the payment app.
  net::EmbeddedTestServer alicepay_;

  // https://bobpay.com/webpay does not permit any other origin to use this
  // payment method.
  net::EmbeddedTestServer bobpay_;

  // https://frankpay.com/webpay supports payment apps from any origin.
  net::EmbeddedTestServer frankpay_;

  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestPaymentAppTest);
};

// Test payment request methods are not supported by the payment app.
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentAppTest, NotSupportedError) {
  InstallPaymentAppInScopeForMethod(GetAlicePayInstallURL("/app1/"),
                                    "https://frankpay.com");

  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo("/payment_request_bobpay_test.html");

    ResetEventObserverForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                   DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
    ExpectBodyContains({"false"});

    // A new payment request will be created below, so call
    // SetDownloaderAndIgnorePortInAppScopeForTesting again.
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    ResetEventObserver(DialogEvent::NOT_SUPPORTED_ERROR);
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "buy();"));
    WaitForObservedEvent();
    ExpectBodyContains({"NotSupportedError"});
  }

  // Repeat should have identical results.
  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo("/payment_request_bobpay_test.html");

    ResetEventObserverForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                   DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
    ExpectBodyContains({"false"});

    // A new payment request will be created below, so call
    // SetDownloaderAndIgnorePortInAppScopeForTesting again.
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    ResetEventObserver(DialogEvent::NOT_SUPPORTED_ERROR);
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "buy();"));
    WaitForObservedEvent();
    ExpectBodyContains({"NotSupportedError"});
  }
}

// Test CanMakePayment and payment request can be fullfiled.
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentAppTest, PayWithAlicePay) {
  InstallPaymentAppInScopeForMethod(GetAlicePayInstallURL("/app1/"),
                                    "https://alicepay.com");

  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo("/payment_request_bobpay_test.html");

    ResetEventObserverForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                   DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
    ExpectBodyContains({"true"});

    // A new payment request will be created below, so call
    // SetDownloaderAndIgnorePortInAppScopeForTesting again.
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    InvokePaymentRequestUI();

    ResetEventObserver(DialogEvent::DIALOG_CLOSED);
    ClickOnDialogViewAndWait(DialogViewID::PAY_BUTTON, dialog_view());
    ExpectBodyContains({"https://alicepay.com"});
  }

  // Repeat should have identical results.
  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo("/payment_request_bobpay_test.html");

    ResetEventObserverForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                   DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
    ExpectBodyContains({"true"});

    // A new payment request will be created below, so call
    // SetDownloaderAndIgnorePortInAppScopeForTesting again.
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    InvokePaymentRequestUI();

    ResetEventObserver(DialogEvent::DIALOG_CLOSED);
    ClickOnDialogViewAndWait(DialogViewID::PAY_BUTTON, dialog_view());
    ExpectBodyContains({"https://alicepay.com"});
  }
}

// Test https://bobpay.com can not be used by https://alicepay.com
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentAppTest, CanNotPayWithBobPay) {
  InstallPaymentAppInScopeForMethod(GetAlicePayInstallURL("/app1/"),
                                    "https://bobpay.com");

  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo("/payment_request_bobpay_test.html");

    ResetEventObserverForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                   DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
    ExpectBodyContains({"false"});

    // A new payment request will be created below, so call
    // SetDownloaderAndIgnorePortInAppScopeForTesting again.
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    ResetEventObserver(DialogEvent::NOT_SUPPORTED_ERROR);
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "buy();"));
    WaitForObservedEvent();
    ExpectBodyContains({"NotSupportedError"});
  }

  // Repeat should have identical results.
  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo("/payment_request_bobpay_test.html");

    ResetEventObserverForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                   DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
    ExpectBodyContains({"false"});

    // A new payment request will be created below, so call
    // SetDownloaderAndIgnorePortInAppScopeForTesting again.
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    ResetEventObserver(DialogEvent::NOT_SUPPORTED_ERROR);
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "buy();"));
    WaitForObservedEvent();
    ExpectBodyContains({"NotSupportedError"});
  }
}

// Test can pay with 'basic-card' payment method from alicepay.
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentAppTest, PayWithBasicCard) {
  InstallPaymentAppInScopeForMethod(GetAlicePayInstallURL("/app1/"),
                                    "basic-card");

  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo(
        "/payment_request_bobpay_and_basic_card_with_modifiers_test.html");
    InvokePaymentRequestUI();

    ResetEventObserver(DialogEvent::DIALOG_CLOSED);
    ClickOnDialogViewAndWait(DialogViewID::PAY_BUTTON, dialog_view());
    ExpectBodyContains({"basic-card"});
  }

  // Repeat should have identical results.
  {
    SetDownloaderAndIgnorePortInAppScopeForTesting();

    NavigateTo(
        "/payment_request_bobpay_and_basic_card_with_modifiers_test.html");
    InvokePaymentRequestUI();

    ResetEventObserver(DialogEvent::DIALOG_CLOSED);
    ClickOnDialogViewAndWait(DialogViewID::PAY_BUTTON, dialog_view());
    ExpectBodyContains({"basic-card"});
  }
}

}  // namespace payments
