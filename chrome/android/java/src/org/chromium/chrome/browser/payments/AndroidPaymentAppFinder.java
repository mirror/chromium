// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ResolveInfo;
import android.text.TextUtils;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.payments.PaymentAppFactory.PaymentAppCreatedCallback;
import org.chromium.chrome.browser.payments.PaymentManifestVerifier.ManifestVerifyCallback;
import org.chromium.components.payments.PaymentManifestDownloader;
import org.chromium.components.payments.PaymentManifestParser;
import org.chromium.content_public.browser.WebContents;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * Finds installed native Android payment apps and verifies their signatures according to the
 * payment method manifests. The manifests are located based on the payment method name, which is a
 * URI that starts with "https://". The W3C-published non-URI payment method names are exceptions:
 * these are common payment method names that do not have a manifest and can be used by any payment
 * app.
 */
public class AndroidPaymentAppFinder implements ManifestVerifyCallback {
    private static final String TAG = "PaymentAppFinder";

    /** The maximum number of payment method manifests to download. */
    private static final int MAX_NUMBER_OF_MANIFESTS = 10;

    /** The name of the intent for the service to check whether an app is ready to pay. */
    /* package */ static final String ACTION_IS_READY_TO_PAY =
            "org.chromium.intent.action.IS_READY_TO_PAY";

    /**
     * Meta data name of an app's supported payment method names.
     */
    /* package */ static final String META_DATA_NAME_OF_PAYMENT_METHOD_NAMES =
            "org.chromium.payment_method_names";

    /**
     * Meta data name of an app's supported default payment method name.
     */
    /* package */ static final String META_DATA_NAME_OF_DEFAULT_PAYMENT_METHOD_NAME =
            "org.chromium.default_payment_method_name";

    private static boolean sAllowHttpForTest;

    private final WebContents mWebContents;
    private final Set<String> mNonUriPaymentMethods;
    private final Set<URI> mUriPaymentMethods;
    private final PaymentManifestDownloader mDownloader;
    private final PaymentManifestWebDataService mWebDataService;
    private final PaymentManifestParser mParser;
    private final PackageManagerDelegate mPackageManagerDelegate;
    private final PaymentAppCreatedCallback mCallback;
    private final boolean mIsIncognito;

    /**
     * A map of non-default URI payment method names to the set of apps claiming support.
     */
    private final Map<URI, Set<ResolveInfo>> mPendingNonDefaultApps = new HashMap<>();

    /**
     * List of payment manifest verifiers. Note that each non basic card payment method has a
     * dedicated payment manifest verifier.
     */
    private final List<PaymentManifestVerifier> mManifestVerifiers = new ArrayList<>();

    /**
     * A map of payment method names to the array of origins of payment apps that are supported.
     */
    private final Map<URI, URI[]> mSupportedOrigins = new HashMap<>();

    /**
     * A map of default payment method name to the payment app that has been verified to have a
     * valid signature, but may not be in any supported origins list.
     */
    private final Map<String, Set<ResolveInfo>> mNonDefaultPaymentApps = new HashMap<>();

    /** A map of Android package name to the verified payment app. */
    private final Map<String, AndroidPaymentApp> mResult = new HashMap<>();

    /**
     * Finds native Android payment apps.
     *
     * @param webContents            The web contents that invoked the web payments API.
     * @param methods                The list of payment methods requested by the merchant. For
     *                               example, "https://bobpay.com", "https://android.com/pay",
     *                               "basic-card".
     * @param webDataService         The web data service to cache manifest.
     * @param downloader             The manifest downloader.
     * @param parser                 The manifest parser.
     * @param packageManagerDelegate The package information retriever.
     * @param callback               The asynchronous callback to be invoked (on the UI thread) when
     *                               all Android payment apps have been found.
     */
    public static void find(WebContents webContents, Set<String> methods,
            PaymentManifestWebDataService webDataService, PaymentManifestDownloader downloader,
            PaymentManifestParser parser, PackageManagerDelegate packageManagerDelegate,
            PaymentAppCreatedCallback callback) {
        new AndroidPaymentAppFinder(webContents, methods, webDataService, downloader, parser,
                packageManagerDelegate, callback)
                .findAndroidPaymentApps();
    }

    private AndroidPaymentAppFinder(WebContents webContents, Set<String> methods,
            PaymentManifestWebDataService webDataService, PaymentManifestDownloader downloader,
            PaymentManifestParser parser, PackageManagerDelegate packageManagerDelegate,
            PaymentAppCreatedCallback callback) {
        mWebContents = webContents;

        // For non-URI payment method names, only names published by W3C should be supported.
        Set<String> supportedNonUriPaymentMethods = new HashSet<>();
        // https://w3c.github.io/webpayments-methods-card/
        supportedNonUriPaymentMethods.add("basic-card");
        // https://w3c.github.io/webpayments/proposals/interledger-payment-method.html
        supportedNonUriPaymentMethods.add("interledger");

        mNonUriPaymentMethods = new HashSet<>();
        mUriPaymentMethods = new HashSet<>();
        for (String method : methods) {
            assert !TextUtils.isEmpty(method);
            if (supportedNonUriPaymentMethods.contains(method)) {
                Log.e(TAG, "non-uri method '%s'", method);
                mNonUriPaymentMethods.add(method);
            } else if (isUriMethod(method)) {
                URI uri = parseAbsoluteUriMethodNameFromString(method);
                if (uri != null) mUriPaymentMethods.add(uri);
            }
        }

        mDownloader = downloader;
        mWebDataService = webDataService;
        mParser = parser;
        mPackageManagerDelegate = packageManagerDelegate;
        mCallback = callback;
        ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
        mIsIncognito = activity != null && activity.getCurrentTabModel() != null
                && activity.getCurrentTabModel().isIncognito();
    }

    /**
     * Checks whether the given <code>method</code> string has the correct format to be a URI
     * payment method name. Does not perform complete URI validation.
     *
     * @param method The payment method name to check.
     * @return Whether the method name has the correct format to be a URI payment method name.
     */
    public static boolean isUriMethod(String method) {
        return method.startsWith(UrlConstants.HTTPS_URL_PREFIX)
                || (sAllowHttpForTest && method.startsWith("http://127.0.0.1:"));
    }

    /**
     * Parses the input <code>method</code> into a URI payment method name. Returns null for
     * invalid URI format or a relative URI.
     *
     * @param method The payment method name to parse.
     * @return The parsed URI payment method name or null if not valid.
     */
    @Nullable
    public static URI parseAbsoluteUriMethodNameFromString(String method) {
        URI uri;
        try {
            // Don't use java.net.URL, because it performs a synchronous DNS lookup in the
            // constructor.
            uri = new URI(method);
        } catch (URISyntaxException e) {
            return null;
        }

        if (!uri.isAbsolute()) return null;

        assert UrlConstants.HTTPS_SCHEME.equals(uri.getScheme())
                || (sAllowHttpForTest
                        && UrlConstants.HTTP_SCHEME.equals(uri.getScheme())
                        && "127.0.0.1".equals(uri.getHost()));

        return uri;
    }

    /** Finds the installed android payment apps. */
    private void findAndroidPaymentApps() {
        Intent payIntent = new Intent(AndroidPaymentApp.ACTION_PAY);
        List<ResolveInfo> apps =
                mPackageManagerDelegate.getActivitiesThatCanRespondToIntentWithMetaData(payIntent);
        if (apps.isEmpty()) {
            Log.e(TAG, "No apps can respond to PAY intent.");
            onSearchFinished();
            return;
        }

        Set<URI> allUriPaymentMethods = new HashSet<>();
        allUriPaymentMethods.addAll(mUriPaymentMethods);

        List<Set<String>> appDefaultSupportedMethods = new ArrayList<>();
        List<Set<String>> appNonDefaultSupportedMethods = new ArrayList<>();
        List<Set<String>> appAllSupportedMethods = new ArrayList<>();
        Map<URI, Set<URI>> nonDefaultAppOrigins = new HashMap<>();

        for (int i = 0; i < apps.size(); i++) {
            Set<String> defaultMethods = getDefaultPaymentMethodName(apps.get(i).activityInfo);
            appDefaultSupportedMethods.add(defaultMethods);
            URI appDefaultUriMethodName = null;
            if (!defaultMethods.isEmpty()) {
                assert defaultMethods.size() == 1;
                String method = defaultMethods.iterator().next();
                if (isUriMethod(method)) {
                    appDefaultUriMethodName = parseAbsoluteUriMethodNameFromString(method);
                    if (appDefaultUriMethodName != null) {
                        allUriPaymentMethods.add(appDefaultUriMethodName);
                    }
                }
            }

            Set<String> nonDefaultMethods = getNonDefaultPaymentMethodNames(
                    apps.get(i).activityInfo);
            appNonDefaultSupportedMethods.add(nonDefaultMethods);

            Set<String> allMethods = new HashSet<>();
            allMethods.addAll(defaultMethods);
            allMethods.addAll(nonDefaultMethods);
            appAllSupportedMethods.add(allMethods);

            if (appDefaultUriMethodName != null) {
                for (String method : nonDefaultMethods) {
                    if (isUriMethod(method)) {
                        URI uriMethod = parseAbsoluteUriMethodNameFromString(method);
                        if (uriMethod != null) {
                            Set<URI> origins = nonDefaultAppOrigins.get(uriMethod);
                            if (origins == null) {
                                origins = new HashSet<>();
                                nonDefaultAppOrigins.put(uriMethod, origins);
                            }
                            origins.add(appDefaultUriMethodName);
                        }
                    }
                }
            }
        }

        for (URI uriMethodName : allUriPaymentMethods) {
            if (filterAppsByMethodName(
                    apps, appAllSupportedMethods, uriMethodName.toString()).isEmpty()) {
                Log.e(TAG, "no apps for '%s'", uriMethodName);
                continue;
            }
            Log.e(TAG, "found apps for '%s'", uriMethodName);

            // Start the parser utility process as soon as possible, once we know that a manifest
            // file needs to be parsed. The startup can take up to 2 seconds.
            if (!mParser.isUtilityProcessRunning()) mParser.startUtilityProcess();

            // Initialize the native side of the downloader, once we know that a manifest file needs
            // to be downloaded.
            if (!mDownloader.isInitialized()) mDownloader.initialize(mWebContents);

            List<ResolveInfo> defaultSupportedApps = filterAppsByMethodName(
                    apps, appDefaultSupportedMethods, uriMethodName.toString());
            mManifestVerifiers.add(new PaymentManifestVerifier(
                    uriMethodName, defaultSupportedApps,
                    nonDefaultAppOrigins.get(uriMethodName), mWebDataService, mDownloader, mParser,
                    mPackageManagerDelegate, this /* callback */));
            mPendingNonDefaultApps.put(uriMethodName, new HashSet<>(
                    filterAppsByMethodName(apps, appNonDefaultSupportedMethods,
                            uriMethodName.toString())));

            if (mManifestVerifiers.size() == MAX_NUMBER_OF_MANIFESTS) {
                Log.e(TAG, "Reached maximum number of allowed payment app manifests.");
                break;
            }
        }

        for (String nonUriMethodName : mNonUriPaymentMethods) {
            List<ResolveInfo> supportedApps =
                    filterAppsByMethodName(apps, appAllSupportedMethods, nonUriMethodName);
            mNonDefaultPaymentApps.put(nonUriMethodName, new HashSet<>(supportedApps));
            for (int i = 0; i < supportedApps.size(); i++) {
                // Chrome does not verify app manifests for non-URI payment method support.
                onValidPaymentAppForPaymentMethodName(nonUriMethodName, supportedApps.get(i));
            }
        }

        if (mManifestVerifiers.isEmpty()) {
            onSearchFinished();
            return;
        }

        for (int i = 0; i < mManifestVerifiers.size(); i++) {
            mManifestVerifiers.get(i).verify();
        }
    }

    private Set<String> getDefaultPaymentMethodName(ActivityInfo activityInfo) {
        Set<String> result = new HashSet<>();
        if (activityInfo.metaData == null) return result;
        String defaultMethodName =
                activityInfo.metaData.getString(META_DATA_NAME_OF_DEFAULT_PAYMENT_METHOD_NAME);
        if (!TextUtils.isEmpty(defaultMethodName)) result.add(defaultMethodName);
        return result;
    }

    private Set<String> getNonDefaultPaymentMethodNames(ActivityInfo activityInfo) {
        Set<String> result = new HashSet<>();
        if (activityInfo.metaData == null) return result;
        int resId = activityInfo.metaData.getInt(META_DATA_NAME_OF_PAYMENT_METHOD_NAMES);
        if (resId == 0) return result;
        String[] nonDefaultPaymentMethodNames =
                mPackageManagerDelegate.getStringArrayResourceForApplication(
                        activityInfo.applicationInfo, resId);
        if (nonDefaultPaymentMethodNames == null) return result;
        for (int i = 0; i < nonDefaultPaymentMethodNames.length; i++) {
            result.add(nonDefaultPaymentMethodNames[i]);
        }
        return result;
    }

    // Note that apps and  methodNames must have the same size. The information at the same index
    // must correspond to the same app.
    private static List<ResolveInfo> filterAppsByMethodName(
            List<ResolveInfo> apps, List<Set<String>> methodNames, String targetMethodName) {
        assert apps.size() == methodNames.size();
        List<ResolveInfo> supportedApps = new ArrayList<>();
        for (int i = 0; i < apps.size(); i++) {
            if (methodNames.get(i).contains(targetMethodName)) {
                supportedApps.add(apps.get(i));
                continue;
            }
        }
        return supportedApps;
    }

    @Override
    public void onValidPaymentApp(URI methodName, ResolveInfo resolveInfo) {
        Log.e(TAG, "'%s' is valid app for '%s'.", resolveInfo.activityInfo.packageName,
                methodName);
        if (mUriPaymentMethods.contains(methodName)) {
            onValidPaymentAppForPaymentMethodName(methodName.toString(), resolveInfo);
        }

        if (!mNonDefaultPaymentApps.containsKey(methodName.toString())) {
            mNonDefaultPaymentApps.put(methodName.toString(), new HashSet<ResolveInfo>());
        }
        Set<ResolveInfo> nonDefaultPaymentApps = mNonDefaultPaymentApps.get(
                methodName.toString());
        nonDefaultPaymentApps.add(resolveInfo);
        verifyUriAppsThroughSupportedOrigins();
    }

    /** Same as above, but also works for non-URI method names, e.g., "basic-card". */
    private void onValidPaymentAppForPaymentMethodName(String methodName, ResolveInfo resolveInfo) {
        String packageName = resolveInfo.activityInfo.packageName;
        AndroidPaymentApp app = mResult.get(packageName);
        if (app == null) {
            CharSequence label = mPackageManagerDelegate.getAppLabel(resolveInfo);
            if (TextUtils.isEmpty(label)) {
                Log.e(TAG, "Skipping \"%s\" because of empty label.", packageName);
                return;
            }
            app = new AndroidPaymentApp(mWebContents, packageName,
                    resolveInfo.activityInfo.name,
                    label.toString(), mPackageManagerDelegate.getAppIcon(resolveInfo),
                    mIsIncognito);
            mResult.put(packageName, app);
        }
        app.addMethodName(methodName);
    }

    @Override
    public void onInvalidPaymentApp(URI methodName, ResolveInfo resolveInfo) {}

    @Override
    public void onInvalidManifest(URI methodName) {}

    @Override
    public void onSupportedOrigins(URI methodName, URI[] supportedOrigins) {
        assert !mSupportedOrigins.containsKey(methodName);
        Log.e(TAG, "'%s' supports %d origins.", methodName, supportedOrigins.length);
        mSupportedOrigins.put(methodName, supportedOrigins);
        verifyUriAppsThroughSupportedOrigins();
    }

    @Override
    public void onAllOriginsSupported(URI methodName) {
        Log.e(TAG, "supports all origins: '%s'", methodName);
        Set<ResolveInfo> apps = mPendingNonDefaultApps.get(methodName);
        if (apps == null) {
            Log.e(TAG, "no apps use payment method '%s'", methodName);
            return;
        }
        Log.e(TAG, "%d apps use payment method '%s'", apps.size(), methodName);
        for (ResolveInfo app : apps) {
            onValidPaymentApp(methodName, app);
        }
        mPendingNonDefaultApps.remove(methodName);
    }

    private void verifyUriAppsThroughSupportedOrigins() {
        // https://bobpay.com/webpay -> BobPay.
        for (Map.Entry<String, Set<ResolveInfo>> methodAndApps :
                mNonDefaultPaymentApps.entrySet()) {
            String appDefaultMethodName = methodAndApps.getKey();
            Log.e(TAG, "app method=%s", appDefaultMethodName);
            if (!isUriMethod(appDefaultMethodName)) continue;

            URI appDefaultUriMethodName = parseAbsoluteUriMethodNameFromString(appDefaultMethodName);
            if (appDefaultUriMethodName == null) continue;

            // Tests use sub-directories to simulate different hosts, because the test web server
            // runs on a single localhost origin.
            URI appOrigin = appDefaultUriMethodName.resolve(sAllowHttpForTest ? "." : "/");

            Log.e(TAG, "app origin=%s", appOrigin);
            Set<ResolveInfo> appsClaimingSupport = methodAndApps.getValue();

            // https://georgepay.com/webpay -> [https://bobpay.com].
            for (Map.Entry<URI, URI[]> paymentMethodSupportedOrigins
                    : mSupportedOrigins.entrySet()) {
                URI paymentMethod = paymentMethodSupportedOrigins.getKey();
                URI[] supportedOrigins = paymentMethodSupportedOrigins.getValue();
                for (int i = 0; i < supportedOrigins.length; i++) {
                    Log.e(TAG, "supported origin=%s", supportedOrigins[i]);
                    if (areSameOrigin(appOrigin, supportedOrigins[i])) {
                        for (ResolveInfo appClaimingSupport : appsClaimingSupport) {
                            onValidPaymentAppForPaymentMethodName(
                                    paymentMethod.toString(), appClaimingSupport);
                        }
                    }
                }
            }
        }
    }

    private static boolean areSameOrigin(URI a, URI b) {
        if (!a.getScheme().equals(b.getScheme())) return false;
        if (!a.getHost().equals(b.getHost())) return false;
        if (a.getPort() != b.getPort()) return false;

        if (!sAllowHttpForTest) return true;

        return removeTrailingSlash(a.getPath()).equals(removeTrailingSlash(b.getPath()));
    }

    private static String removeTrailingSlash(String path) {
        return path.isEmpty() || path.charAt(path.length() - 1) != '/' ?  path : path.substring
                (0, path.length() - 1);
    }

    @Override
    public void onVerifyFinished(PaymentManifestVerifier verifier) {
        Log.e(TAG, "verifier finished.");
        mManifestVerifiers.remove(verifier);
        if (!mManifestVerifiers.isEmpty()) return;

        mWebDataService.destroy();
        if (mDownloader.isInitialized()) mDownloader.destroy();
        if (mParser.isUtilityProcessRunning()) mParser.stopUtilityProcess();
        onSearchFinished();
    }

    /**
     * Checks for IS_READY_TO_PAY service in each valid payment app and returns the valid apps
     * to the caller. Called when finished verifying all payment methods and apps.
     */
    private void onSearchFinished() {
        if (!mIsIncognito) {
            List<ResolveInfo> resolveInfos =
                    mPackageManagerDelegate.getServicesThatCanRespondToIntent(
                            new Intent(ACTION_IS_READY_TO_PAY));
            for (int i = 0; i < resolveInfos.size(); i++) {
                ResolveInfo resolveInfo = resolveInfos.get(i);
                AndroidPaymentApp app = mResult.get(resolveInfo.serviceInfo.packageName);
                if (app != null) app.setIsReadyToPayAction(resolveInfo.serviceInfo.name);
            }
        }

        for (Map.Entry<String, AndroidPaymentApp> entry : mResult.entrySet()) {
            mCallback.onPaymentAppCreated(entry.getValue());
        }

        mCallback.onAllPaymentAppsCreated();
    }

    @VisibleForTesting
    public static void allowHttpForTest() {
        sAllowHttpForTest = true;
    }
}