// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * A payment integration test for service worker based payment apps.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
        "disable-features=" + ChromeFeatureList.WEB_PAYMENTS_SINGLE_APP_UI_SKIP,
        "enable-features=" + ChromeFeatureList.NO_CREDIT_CARD_ABORT,
})
public class PaymentRequestServiceWorkerPaymentAppTest {
    @Rule
    public PaymentRequestTestRule mPaymentRequestTestRule = new PaymentRequestTestRule(
            "payment_request_bobpay_and_basic_card_with_modifiers_test.html");

    /**
     * Installs a mock service worker based payment app for testing.
     *
     * @param supportedMethodNames The supported payment methods of the mock payment app.
     */
    private void installMockServiceWorkerPaymentApp(final String[] supportedMethodNames) {
        PaymentAppFactory.getInstance().addAdditionalFactory(
                (webContents, methodNames, callback) -> {
                    callback.onPaymentAppCreated(
                            new ServiceWorkerPaymentApp(webContents, 0 /* registrationId */,
                                    UriUtils.parseUriFromString("https://bobpay.com") /* scope */,
                                    "BobPay" /* label */, null /* sublabel*/,
                                    "https://bobpay.com" /* tertiarylabel */, null /* icon */,
                                    supportedMethodNames /* methodNames */,
                                    new ServiceWorkerPaymentApp.Capabilities[0] /* capabilities */,
                                    new String[0] /* preferredRelatedApplicationIds */));
                    callback.onAllPaymentAppsCreated();
                });
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testNoSupportedPaymentMethods()
            throws InterruptedException, ExecutionException, TimeoutException {
        installMockServiceWorkerPaymentApp(new String[0]);

        ServiceWorkerPaymentAppBridge.setCanMakePaymentForTesting(true);

        mPaymentRequestTestRule.openPageAndClickBuyAndWait(mPaymentRequestTestRule.getShowFailed());
        mPaymentRequestTestRule.expectResultContains(
                new String[] {"show() rejected", "The payment method", "not supported"});
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testHasSupportedPaymentMethods()
            throws InterruptedException, ExecutionException, TimeoutException {
        String[] supportedMethodNames = {"https://bobpay.com"};
        installMockServiceWorkerPaymentApp(supportedMethodNames);

        ServiceWorkerPaymentAppBridge.setCanMakePaymentForTesting(true);

        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        Assert.assertEquals(1, mPaymentRequestTestRule.getNumberOfPaymentInstruments());
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testDoNotCallCanMakePayment()
            throws InterruptedException, ExecutionException, TimeoutException {
        String[] supportedMethodNames = {"basic-card"};
        installMockServiceWorkerPaymentApp(supportedMethodNames);

        // Sets setCanMakePaymentForTesting(false) to return false for CanMakePayment since there is
        // no real sw payment app, so if CanMakePayment is called then no payment instruments will
        // be available, otherwise CanMakePayment is not called.
        ServiceWorkerPaymentAppBridge.setCanMakePaymentForTesting(false);

        mPaymentRequestTestRule.triggerUIAndWait(mPaymentRequestTestRule.getReadyForInput());
        Assert.assertEquals(1, mPaymentRequestTestRule.getNumberOfPaymentInstruments());
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testCallCanMakePayment()
            throws InterruptedException, ExecutionException, TimeoutException {
        String[] supportedMethodNames = {"https://bobpay.com", "basic-card"};
        installMockServiceWorkerPaymentApp(supportedMethodNames);

        // Sets setCanMakePaymentForTesting(false) to return false for CanMakePayment since there is
        // no real sw payment app, so if CanMakePayment is called then no payment instruments will
        // be available, otherwise CanMakePayment is not called.
        ServiceWorkerPaymentAppBridge.setCanMakePaymentForTesting(false);

        mPaymentRequestTestRule.openPageAndClickBuyAndWait(mPaymentRequestTestRule.getShowFailed());
        mPaymentRequestTestRule.expectResultContains(
                new String[] {"show() rejected", "The payment method", "not supported"});
    }
}
