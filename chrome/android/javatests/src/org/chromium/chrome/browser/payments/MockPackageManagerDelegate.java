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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Simulates a package manager in memory. */
class MockPackageManagerDelegate extends PackageManagerDelegate {
    private final List<ResolveInfo> mActivities = new ArrayList<>();
    private final Map<String, PackageInfo> mPackages = new HashMap<>();
    private final Map<ResolveInfo, CharSequence> mLabels = new HashMap<>();

    /**
     * Simulates an installed app.
     *
     * @param label                   The user visible name of the app.
     * @param packageName             The identifying package name.
     * @param defaultPaymentMethoName The name of the default payment method name for this app.
     * @param signature               The signature of the app. The SHA256 hash of this
     *                                signature is called "fingerprint" and should be present in
     *                                the app's web app manifest.
     */
    public void installPaymentApp(CharSequence label, String packageName,
            String defaultPaymentMethoName, String signature) {
        ResolveInfo paymentApp = new ResolveInfo();
        paymentApp.activityInfo = new ActivityInfo();
        paymentApp.activityInfo.packageName = packageName;
        paymentApp.activityInfo.name = packageName + ".WebPaymentActivity";
        paymentApp.activityInfo.applicationInfo = new ApplicationInfo();
        Bundle metaData = new Bundle();
        metaData.putString(AndroidPaymentAppFinder.META_DATA_NAME_OF_DEFAULT_PAYMENT_METHOD_NAME,
                defaultPaymentMethoName);
        paymentApp.activityInfo.metaData = metaData;
        mActivities.add(paymentApp);

        PackageInfo packageInfo = new PackageInfo();
        packageInfo.versionCode = 10;
        packageInfo.signatures = new Signature[1];
        packageInfo.signatures[0] = new Signature(signature);
        mPackages.put(packageName, packageInfo);

        mLabels.put(paymentApp, label);
    }

    /** Resets the package manager to the state of no installed apps. */
    public void reset() {
        mActivities.clear();
        mPackages.clear();
        mLabels.clear();
    }

    @Override
    public List<ResolveInfo> getActivitiesThatCanRespondToIntentWithMetaData(Intent intent) {
        return mActivities;
    }

    @Override
    public List<ResolveInfo> getActivitiesThatCanRespondToIntent(Intent intent) {
        return getActivitiesThatCanRespondToIntentWithMetaData(intent);
    }

    @Override
    public List<ResolveInfo> getServicesThatCanRespondToIntent(Intent intent) {
        return new ArrayList<>();
    }

    @Override
    public PackageInfo getPackageInfoWithSignatures(String packageName) {
        return mPackages.get(packageName);
    }

    @Override
    public CharSequence getAppLabel(ResolveInfo resolveInfo) {
        return mLabels.get(resolveInfo);
    }

    @Override
    public Drawable getAppIcon(ResolveInfo resolveInfo) {
        return null;
    }
}