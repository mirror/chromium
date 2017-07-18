// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import junit.framework.Assert;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.payments.PaymentAppFactory.PaymentAppCreatedCallback;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.payments.PaymentManifestDownloader;
import org.chromium.components.payments.PaymentManifestParser;
import org.chromium.components.payments.PaymentManifestParser.ManifestParseCallback;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.payments.mojom.WebAppManifestSection;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** An integration test for the Android payment app finder. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
})
@MediumTest
public class AndroidPaymentAppFinderTest implements PaymentAppCreatedCallback {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    /**
     * Wrapper around a payment manifest parser that substitutes web app manifest URIs to point to
     * the test server.
     */
    private static class URLSubstituteParser extends PaymentManifestParser {
        private URI mTestServerUri;

        /** @param uri The URI of the test server. */
        public void setTestServerUri(URI uri) {
            assert mTestServerUri == null : "Test server URI should be set only once";
            mTestServerUri = uri;
        }

        @Override
        public void parsePaymentMethodManifest(
                String content, final ManifestParseCallback callback) {
            super.parsePaymentMethodManifest(content, new ManifestParseCallback() {
                @Override
                public void onPaymentMethodManifestParseSuccess(URI[] webAppManifestUris) {
                    URI[] localhostWebAppManifestUris = new URI[webAppManifestUris.length];
                    for (int i = 0; i < webAppManifestUris.length; i++) {
                        try {
                            localhostWebAppManifestUris[i] = new URI(
                                    webAppManifestUris[i]
                                            .toString()
                                            .replaceAll("https://alicepay.com",
                                                    mTestServerUri.toString() + "/alicepay.com")
                                            .replaceAll("https://bobpay.com",
                                                    mTestServerUri.toString() + "/bobpay.com")
                                            .replaceAll("https://charliepay.com",
                                                    mTestServerUri.toString() + "/charliepay.com"));
                        } catch (URISyntaxException e) {
                            assert false : "URI should be valid";
                        }
                    }
                    callback.onPaymentMethodManifestParseSuccess(localhostWebAppManifestUris);
                }

                @Override
                public void onWebAppManifestParseSuccess(WebAppManifestSection[] manifest) {
                    assert false : "Web app manifest parsing callback should not be triggered.";
                }

                @Override
                public void onManifestParseFailure() {
                    callback.onManifestParseFailure();
                }
            });
        }
    }

    private final URLSubstituteParser mParser = new URLSubstituteParser();

    /** Simulates a package manager in memory. */
    private final MockPackageManagerDelegate mPackageManager = new MockPackageManagerDelegate();

    /** A downloader that allows HTTP URLs. */
    private final PaymentManifestDownloader mDownloader = new PaymentManifestDownloader() {
        @Override
        public void initialize(WebContents webContents) {
            super.initialize(webContents);
            allowHttpForTest();
        }
    };

    private EmbeddedTestServer mServer;
    private List<PaymentApp> mPaymentApps;
    private boolean mAllPaymentAppsCreated;

    // PaymentAppCreatedCallback
    @Override
    public void onPaymentAppCreated(PaymentApp paymentApp) {
        mPaymentApps.add(paymentApp);
    }

    // PaymentAppCreatedCallback
    @Override
    public void onAllPaymentAppsCreated() {
        mAllPaymentAppsCreated = true;
    }

    @Before
    public void setUp() throws Throwable {
        mRule.startMainActivityOnBlankPage();
        mPackageManager.reset();
        mServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());
        mParser.setTestServerUri(new URI(mServer.getURL("/chrome/test/data/payments")));
        AndroidPaymentAppFinder.allowHttpForTest();
        mPaymentApps = new ArrayList<>();
        mAllPaymentAppsCreated = false;
    }

    @After
    public void tearDown() throws Throwable {
        if (mServer != null) mServer.stopAndDestroyServer();
    }

    @Test
    @Feature({"Payments"})
    public void testNoApps() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add("basic-card");
        methods.add(mServer.getURL("/chrome/test/data/payments/alicepay.com/webpay"));
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        methods.add(mServer.getURL("/chrome/test/data/payments/charliepay.com/webpay"));
        methods.add(mServer.getURL("/chrome/test/data/payments/davepay.com/webpay"));

        findApps(methods);

        Assert.assertTrue("No apps should match the query", mPaymentApps.isEmpty());
    }

    @Test
    @Feature({"Payments"})
    public void testTwoAppsWithIncorrectMethodNames() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add("basic-card");
        mPackageManager.installPaymentApp(
                "BobPay", "com.bobpay", "incorrect-method-name", "01020304050607080900");
        mPackageManager.installPaymentApp(
                "AlicePay", "com.alicepay", "incorrect-method-name", "ABCDEFABCDEFABCDEFAB");

        findApps(methods);

        Assert.assertTrue("No apps should match the query", mPaymentApps.isEmpty());
    }

    @Test
    @Feature({"Payments"})
    public void testOneBasicCardApp() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add("basic-card");
        mPackageManager.installPaymentApp(
                "BobPay", "com.bobpay", "basic-card", "01020304050607080900");

        findApps(methods);

        Assert.assertEquals("1 app should match the query", 1, mPaymentApps.size());
        Assert.assertEquals("com.bobpay", mPaymentApps.get(0).getAppIdentifier());
    }

    @Test
    @Feature({"Payments"})
    public void testOneUrlMethodNameApp() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                "01020304050607080900");

        findApps(methods);

        Assert.assertEquals("1 app should match the query", 1, mPaymentApps.size());
        Assert.assertEquals("com.bobpay", mPaymentApps.get(0).getAppIdentifier());
    }

    @Test
    @Feature({"Payments"})
    public void testOneUrlMethodNameAppWithWrongSignature() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                "AA" /* incorrect signature */);

        findApps(methods);

        Assert.assertTrue("No apps should match the query", mPaymentApps.isEmpty());
    }

    @Test
    @Feature({"Payments"})
    public void testOneAppWithoutPackageInfo() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                null /* no package info*/);

        findApps(methods);

        Assert.assertTrue("No apps should match the query", mPaymentApps.isEmpty());
    }

    @Test
    @Feature({"Payments"})
    public void testOneAppWithoutSignatures() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                "" /* no signatures */);

        findApps(methods);

        Assert.assertTrue("No apps should match the query", mPaymentApps.isEmpty());
    }

    @Test
    @Feature({"Payments"})
    public void testTwoBasicCardApps() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add("basic-card");
        mPackageManager.installPaymentApp(
                "BobPay", "com.bobpay", "basic-card", "01020304050607080900");
        mPackageManager.installPaymentApp(
                "AlicePay", "com.alicepay", "basic-card", "ABCDEFABCDEFABCDEFAB");

        findApps(methods);

        Assert.assertEquals("2 apps should match the query", 2, mPaymentApps.size());
        Set<String> appIdentifiers = new HashSet<>();
        appIdentifiers.add(mPaymentApps.get(0).getAppIdentifier());
        appIdentifiers.add(mPaymentApps.get(1).getAppIdentifier());
        Assert.assertTrue(appIdentifiers.contains("com.bobpay"));
        Assert.assertTrue(appIdentifiers.contains("com.alicepay"));
    }

    @Test
    @Feature({"Payments"})
    public void testTwoUrlMethodNameApps() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        methods.add(mServer.getURL("/chrome/test/data/payments/alicepay.com/webpay"));
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                "01020304050607080900");
        mPackageManager.installPaymentApp("AlicePay", "com.alicepay",
                mServer.getURL("/chrome/test/data/payments/alicepay.com/webpay"),
                "ABCDEFABCDEFABCDEFAB");

        findApps(methods);

        Assert.assertEquals("2 apps should match the query", 2, mPaymentApps.size());
        Set<String> appIdentifiers = new HashSet<>();
        appIdentifiers.add(mPaymentApps.get(0).getAppIdentifier());
        appIdentifiers.add(mPaymentApps.get(1).getAppIdentifier());
        Assert.assertTrue(appIdentifiers.contains("com.bobpay"));
        Assert.assertTrue(appIdentifiers.contains("com.alicepay"));
    }

    @Test
    @Feature({"Payments"})
    public void testTwoUrlMethodNameAppsTwice() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"));
        methods.add(mServer.getURL("/chrome/test/data/payments/alicepay.com/webpay"));
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                "01020304050607080900");
        mPackageManager.installPaymentApp("AlicePay", "com.alicepay",
                mServer.getURL("/chrome/test/data/payments/alicepay.com/webpay"),
                "ABCDEFABCDEFABCDEFAB");

        findApps(methods);

        Assert.assertEquals("2 apps should match the query", 2, mPaymentApps.size());
        Set<String> appIdentifiers = new HashSet<>();
        appIdentifiers.add(mPaymentApps.get(0).getAppIdentifier());
        appIdentifiers.add(mPaymentApps.get(1).getAppIdentifier());
        Assert.assertTrue(appIdentifiers.contains("com.bobpay"));
        Assert.assertTrue(appIdentifiers.contains("com.alicepay"));

        mPaymentApps.clear();
        mAllPaymentAppsCreated = false;

        findApps(methods);

        Assert.assertEquals("2 apps should still match the query", 2, mPaymentApps.size());
        appIdentifiers.clear();
        appIdentifiers.add(mPaymentApps.get(0).getAppIdentifier());
        appIdentifiers.add(mPaymentApps.get(1).getAppIdentifier());
        Assert.assertTrue(appIdentifiers.contains("com.bobpay"));
        Assert.assertTrue(appIdentifiers.contains("com.alicepay"));
    }

    /**
     * Test CharliePay Dev with https://charliepay.com/webpay payment method, which supports both
     * dev and prod versions of the app. Multiple app look ups in sequence should be successful.
     */
    @Test
    @Feature({"Payments"})
    public void testCharliePayDev() throws Throwable {
        Set<String> methods = new HashSet<>();
        methods.add(mServer.getURL("/chrome/test/data/payments/charliepay.com/webpay"));
        mPackageManager.installPaymentApp("CharliePay", "com.charliepay.dev",
                mServer.getURL("/chrome/test/data/payments/charliepay.com/webpay"),
                "ABCDEFABCDEFABCDEFAB");
        mPackageManager.installPaymentApp("BobPay", "com.bobpay",
                mServer.getURL("/chrome/test/data/payments/bobpay.com/webpay"),
                "ABCDEFABCDEFABCDEFAB");
        mPackageManager.installPaymentApp("AlicePay", "com.alicepay",
                mServer.getURL("/chrome/test/data/payments/alicepay.com/webpay"),
                "ABCDEFABCDEFABCDEFAB");


        findApps(methods);

        Assert.assertEquals("1 app should match the query", 1, mPaymentApps.size());
        Assert.assertEquals("com.charliepay.dev", mPaymentApps.get(0).getAppIdentifier());

        mPaymentApps.clear();
        mAllPaymentAppsCreated = false;

        findApps(methods);

        Assert.assertEquals("1 app should match the query again", 1, mPaymentApps.size());
        Assert.assertEquals("com.charliepay.dev", mPaymentApps.get(0).getAppIdentifier());
    }

    private void findApps(final Set<String> methodNames) throws Throwable {
        mRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AndroidPaymentAppFinder.find(
                        mRule.getActivity().getCurrentContentViewCore().getWebContents(),
                        methodNames, new PaymentManifestWebDataService(), mDownloader, mParser,
                        mPackageManager, AndroidPaymentAppFinderTest.this);
            }
        });
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mAllPaymentAppsCreated;
            }
        });
    }
}