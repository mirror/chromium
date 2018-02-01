// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.Signature;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
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

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

/**
 * A payment integration test for service worker based payment apps with localhost payment method
 * name.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
        PaymentRequestImpl.ALLOW_WEB_PAYMENTS_LOCALHOST_URLS_FLAG,
        "enable-features=" + ChromeFeatureList.SERVICE_WORKER_PAYMENT_APPS,
})
public class PaymentRequestLocalhostServiceWorkerPaymentAppAllowedTest {
    @Rule
    public EmbeddedTestServerRule mServerRule = new EmbeddedTestServerRule();

    @Rule
    public ChromeTabbedActivityTestRule mChromeRule = new ChromeTabbedActivityTestRule();

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testLocalhostServiceWorkerPaymentAppIsAllowedWithCommandLineFlag()
            throws Throwable {
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
                            .contains("Can make payments with \"" + url + "\".");
                } catch (Throwable e) {
                    return false;
                }
            }
        });
    }

    @Test
    @MediumTest
    @Feature({"Payments"})
    public void testLocalhostNativeAndroidPaymentAppIsAllowedWithCommandLineFlag()
            throws Throwable {
        final String methodName = mServerRule.getServer().getURL("/components/test/data/payments/alicepay.com/webpay");

        AndroidPaymentAppFactory.setPackageManagerDelegateForTest(new PackageManagerDelegate() {
            private final PackageManagerDelegate mImpl = new PackageManagerDelegate();

            @Override
            public PackageInfo getPackageInfoWithSignatures(String packageName) {
                android.util.Log.e("chromium",
                        "PackageManagerDelegate.getPackageInfoWithSignatures(\"" + packageName
                                + "\")");
                PackageInfo result = new PackageInfo();
                result.versionCode = 1;
                result.signatures = new Signature[] {new Signature("ABCDEFABCDEFABCDEFAB")};
                return result;
            }

            @Override
            public ResolveInfo resolveActivity(Intent intent) {
                android.util.Log.e("chromium", "PackageManagerDelegate.resolveActivity(intent)");
                return mImpl.resolveActivity(intent);
            }

            @Override
            public List<ResolveInfo> getActivitiesThatCanRespondToIntent(Intent intent) {
                android.util.Log.e("chromium",
                        "PackageManagerDelegate.getActivitiesThatCanRespondToIntent(intent)");
                return mImpl.getActivitiesThatCanRespondToIntent(intent);
            }

            @Override
            public List<ResolveInfo> getActivitiesThatCanRespondToIntentWithMetaData(
                    Intent intent) {
                android.util.Log.e("chromium",
                        "PackageManagerDelegate.getActivitiesThatCanRespondToIntentWithMetaData"
                                + "(intent)");
                ResolveInfo resolveInfo = new ResolveInfo();
                resolveInfo.activityInfo = new ActivityInfo();
                resolveInfo.activityInfo.packageName = "org.chromium.test.payment.app";
                resolveInfo.activityInfo.name = "TestPaymentActivity";
                resolveInfo.activityInfo.metaData = new Bundle();
                resolveInfo.activityInfo.metaData.putString(AndroidPaymentAppFinder.META_DATA_NAME_OF_DEFAULT_PAYMENT_METHOD_NAME, methodName);
                List<ResolveInfo> result = new ArrayList<>();
                result.add(resolveInfo);
                return result;
            }

            @Override
            public List<ResolveInfo> getServicesThatCanRespondToIntent(Intent intent) {
                android.util.Log.e("chromium",
                        "PackageManagerDelegate.getServicesThatCanRespondToIntent(intent)");
                return mImpl.getServicesThatCanRespondToIntent(intent);
            }

            @Override
            public CharSequence getAppLabel(ResolveInfo resolveInfo) {
                android.util.Log.e("chromium", "PackageManagerDelegate.getAppLabel(resolveInfo)");
                return mImpl.getAppLabel(resolveInfo);
            }

            @Override
            public Drawable getAppIcon(ResolveInfo resolveInfo) {
                android.util.Log.e("chromium", "PackageManagerDelegate.getAppIcon(resolveInfo)");
                return mImpl.getAppIcon(resolveInfo);
            }

            @Override
            @Nullable
            public String[] getStringArrayResourceForApplication(
                    ApplicationInfo applicationInfo, int resourceId) {
                android.util.Log.e("chromium",
                        "PackageManagerDelegate.getStringArrayResourceForApplication"
                                + "(applicationInfo, "
                                + Integer.toString(resourceId) + ")");
                return mImpl.getStringArrayResourceForApplication(applicationInfo, resourceId);
            }
        });

        String url =
                mServerRule.getServer().getURL("/components/test/data/payments/alicepay.com/app1/");
        mChromeRule.startMainActivityWithURL(url);
        WebContents webContents =
                mChromeRule.getActivity().getCurrentContentViewCore().getWebContents();

        String script = "canMakePayment('" + methodName + "');";
        JavaScriptUtils.executeJavaScript(webContents, script);
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    return DOMUtils.getNodeContents(webContents, "output")
                            .contains("Can make payments with \"" + methodName + "\".");
                } catch (Throwable e) {
                    return false;
                }
            }
        });
    }
}
