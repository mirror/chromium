// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.support.test.filters.MediumTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.test.EmbeddedTestServerRule;

/**
 * A payment integration test for service worker based payment apps with localhost payment method
 * name.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
        "enable-features=" + ChromeFeatureList.SERVICE_WORKER_PAYMENT_APPS,
})
public class PaymentRequestLocalhostServiceWorkerPaymentAppNotAllowedTest {
    @Rule
    public EmbeddedTestServerRule mServerRule = new EmbeddedTestServerRule();

    @Rule
    public ChromeTabbedActivityTestRule mChromeRule = new ChromeTabbedActivityTestRule();

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testLocalhostPaymentAppIsNotAllowedByDefault() throws Throwable {
        String url =
                mServerRule.getServer().getURL("/components/test/data/payments/alicepay.com/app1/");
        mChromeRule.startMainActivityWithURL(url);
        WebContents webContents =
                mChromeRule.getActivity().getCurrentContentViewCore().getWebContents();
        String script = "install('" + url + "');";
        JavaScriptUtils.executeJavaScript(webContents, script);
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    return DOMUtils.getNodeContents(webContents, "output")
                            .contains("Payment app for \"" + url + "\" method installed.");
                } catch (Throwable e) {
                    return false;
                }
            }
        });
        script = "canMakePayment('" + url + "');";
        JavaScriptUtils.executeJavaScript(webContents, script);
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    return DOMUtils.getNodeContents(webContents, "output")
                            .contains("Invalid payment method identifier format");
                } catch (Throwable e) {
                    return false;
                }
            }
        });
    }
}
