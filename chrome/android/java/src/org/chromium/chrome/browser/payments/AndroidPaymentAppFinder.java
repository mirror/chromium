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
import java.util.Collections;
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

    /** A mapping from an Android package name to the corresponding payment app. */
    private final Map<String, AndroidPaymentApp> mValidApps = new HashMap<>();

    /** A mapping from origins of payment apps to the URI payment methods of these apps. */
    private final Map<URI, Set<URI>> mOriginToMethodsMapping = new HashMap<>();

    /** A mapping from URI payment methods to the applications that support this payment method. */
    private final Map<URI, Set<ResolveInfo>> mMethodToAppsMapping = new HashMap<>();

    /** Contains information about a URL payment method. */
    private static final class PaymentMethod {
        /**
         * The set of yet unverified applications that claim to be the default for this payment
         * method.
         */
        public final Set<ResolveInfo> defaultApplications = new HashSet<>();

        /** The set of yet unverified origins that claim to be supported by this payment method. */
        public final Set<URI> supportedOrigins = new HashSet<>();

        /** Whether all origins are supported. */
        public boolean supportsAllOrigins;
    }

    /** A mapping from URI payment methods to the verified information about these methods. */
    private final Map<URI, PaymentMethod> mVerifiedPaymentMethods = new HashMap<>();

    private int mPendingVerifiersCount;
    private int mPendingResourceUsersCount;

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
                mNonUriPaymentMethods.add(method);
            } else if (looksLikeUriMethod(method)) {
                URI uri = parseUriFromString(method);
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
    public static boolean looksLikeUriMethod(String method) {
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
    public static URI parseUriFromString(String method) {
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
                || (sAllowHttpForTest && UrlConstants.HTTP_SCHEME.equals(uri.getScheme())
                           && "127.0.0.1".equals(uri.getHost()));

        return uri;
    }

    /** Finds the installed android payment apps. */
    private void findAndroidPaymentApps() {
        List<ResolveInfo> allInstalledPaymentApps =
                mPackageManagerDelegate.getActivitiesThatCanRespondToIntentWithMetaData(
                        new Intent(AndroidPaymentApp.ACTION_PAY));
        if (allInstalledPaymentApps.isEmpty()) {
            onAllAppsFound();
            return;
        }

        // All URI methods for which manifests should be downloaded.
        Set<URI> uriMethods = new HashSet<>(mUriPaymentMethods);

        // A mapping from payment method names to the corresponding payment apps.
        Map<String, Set<ResolveInfo>> apps = new HashMap<>();

        // A mapping from payment method names to the corresponding default payment apps.
        Map<String, Set<ResolveInfo>> defaultApps = new HashMap<>();

        // A mapping from URI payment method names to the origins of the payment apps that support
        // that method name.
        Map<URI, Set<URI>> supportedOrigins = new HashMap<>();

        for (URI uriMethod : uriMethods) {
            apps.put(uriMethod.toString(), new HashSet<ResolveInfo>());
            defaultApps.put(uriMethod.toString(), new HashSet<ResolveInfo>());
        }

        for (int i = 0; i < allInstalledPaymentApps.size(); i++) {
            ResolveInfo app = allInstalledPaymentApps.get(i);

            String defaultMethod = app.activityInfo.metaData == null
                    ? null
                    : app.activityInfo.metaData.getString(
                              META_DATA_NAME_OF_DEFAULT_PAYMENT_METHOD_NAME);
            if (TextUtils.isEmpty(defaultMethod)) {
                Log.e(TAG, "Skipping \"%s\" because of missing default payment method name.",
                        app.activityInfo.packageName);
                continue;
            }

            if (!apps.containsKey(defaultMethod)) {
                apps.put(defaultMethod, new HashSet<ResolveInfo>());
            }

            if (!defaultApps.containsKey(defaultMethod)) {
                defaultApps.put(defaultMethod, new HashSet<ResolveInfo>());
            }

            apps.get(defaultMethod).add(app);
            defaultApps.get(defaultMethod).add(app);

            URI appOrigin = null;
            if (looksLikeUriMethod(defaultMethod)) {
                URI defaultUriMethod = parseUriFromString(defaultMethod);
                if (defaultUriMethod != null) {
                    uriMethods.add(defaultUriMethod);

                    appOrigin = getOrigin(defaultUriMethod);
                    if (!mOriginToMethodsMapping.containsKey(appOrigin)) {
                        mOriginToMethodsMapping.put(appOrigin, new HashSet<URI>());
                    }
                    mOriginToMethodsMapping.get(appOrigin).add(defaultUriMethod);
                }
            }

            Set<String> supportedMethods = getSupportedPaymentMethods(app.activityInfo);
            for (String supportedMethod : supportedMethods) {
                if (!apps.containsKey(supportedMethod)) {
                    apps.put(supportedMethod, new HashSet<ResolveInfo>());
                }

                apps.get(supportedMethod).add(app);

                if (!looksLikeUriMethod(supportedMethod)) continue;

                URI supportedUriMethod = parseUriFromString(supportedMethod);
                if (supportedUriMethod == null) continue;

                if (!mMethodToAppsMapping.containsKey(supportedUriMethod)) {
                    mMethodToAppsMapping.put(supportedUriMethod, new HashSet<ResolveInfo>());
                }
                mMethodToAppsMapping.get(supportedUriMethod).add(app);

                if (appOrigin == null) continue;

                if (!supportedOrigins.containsKey(supportedUriMethod)) {
                    supportedOrigins.put(supportedUriMethod, new HashSet<URI>());
                }
                supportedOrigins.get(supportedUriMethod).add(appOrigin);
            }
        }

        List<PaymentManifestVerifier> manifestVerifiers = new ArrayList<>();
        for (URI uriMethodName : uriMethods) {
            if (apps.get(uriMethodName.toString()).isEmpty()) continue;

            // Start the parser utility process as soon as possible, once we know that a manifest
            // file needs to be parsed. The startup can take up to 2 seconds.
            if (!mParser.isUtilityProcessRunning()) mParser.startUtilityProcess();

            // Initialize the native side of the downloader, once we know that a manifest file needs
            // to be downloaded.
            if (!mDownloader.isInitialized()) mDownloader.initialize(mWebContents);

            manifestVerifiers.add(new PaymentManifestVerifier(uriMethodName,
                    defaultApps.get(uriMethodName.toString()), supportedOrigins.get(uriMethodName),
                    mWebDataService, mDownloader, mParser, mPackageManagerDelegate,
                    this /* callback */));

            if (manifestVerifiers.size() == MAX_NUMBER_OF_MANIFESTS) {
                Log.e(TAG, "Reached maximum number of allowed payment app manifests.");
                break;
            }
        }

        for (String nonUriMethodName : mNonUriPaymentMethods) {
            Set<ResolveInfo> supportedApps = apps.get(nonUriMethodName);
            if (supportedApps != null) {
                for (ResolveInfo supportedApp : supportedApps) {
                    // Chrome does not verify app manifests for non-URI payment method support.
                    onValidPaymentAppForPaymentMethodName(supportedApp, nonUriMethodName);
                }
            }
        }

        if (manifestVerifiers.isEmpty()) {
            onAllAppsFound();
            return;
        }

        mPendingVerifiersCount = mPendingResourceUsersCount = manifestVerifiers.size();
        for (PaymentManifestVerifier manifestVerifier : manifestVerifiers) {
            manifestVerifier.verify();
        }
    }

    /**
     * Returns the origin part of the given URI.
     *
     * @param uri The input URI for which the origin needs to be returned. Should not be null.
     * @return The origin of the input URI. Never null.
     */
    private static URI getOrigin(URI uri) {
        assert uri != null;

        // Tests use sub-directories to simulate different hosts, because the test web server runs
        // on a single localhost origin. Therefore, the "origin" in test is
        // https://127.0.0.1:12355/components/test/data/payments/bobpay.xyz instead of
        // https://127.0.0.1:12355.
        String originString = uri.resolve(sAllowHttpForTest ? "." : "/").toString();
        originString =
                originString.isEmpty() || originString.charAt(originString.length() - 1) != '/'
                ? originString
                : originString.substring(0, originString.length() - 1);
        URI origin = parseUriFromString(originString);
        assert origin != null;

        return origin;
    }

    /**
     * Queries the Android app metadata for the names of the non-default payment methods that the
     * given app supports.
     *
     * @param activityInfo The application information to query.
     * @return The set of non-default payment method names that this application supports. Never
     *         null.
     */
    private Set<String> getSupportedPaymentMethods(ActivityInfo activityInfo) {
        Set<String> result = new HashSet<>();
        if (activityInfo.metaData == null) return result;

        int resId = activityInfo.metaData.getInt(META_DATA_NAME_OF_PAYMENT_METHOD_NAMES);
        if (resId == 0) return result;

        String[] nonDefaultPaymentMethodNames =
                mPackageManagerDelegate.getStringArrayResourceForApplication(
                        activityInfo.applicationInfo, resId);
        if (nonDefaultPaymentMethodNames == null) return result;

        Collections.addAll(result, nonDefaultPaymentMethodNames);

        return result;
    }

    @Override
    public void onValidDefaultPaymentApp(URI methodName, ResolveInfo resolveInfo) {
        getOrCreateVerifiedPaymentMethod(methodName).defaultApplications.add(resolveInfo);
    }

    @Override
    public void onValidSupportedOrigin(URI methodName, URI supportedOrigin) {
        getOrCreateVerifiedPaymentMethod(methodName).supportedOrigins.add(supportedOrigin);
    }

    @Override
    public void onAllOriginsSupported(URI methodName) {
        getOrCreateVerifiedPaymentMethod(methodName).supportsAllOrigins = true;
    }

    private PaymentMethod getOrCreateVerifiedPaymentMethod(URI methodName) {
        PaymentMethod verifiedPaymentManifest = mVerifiedPaymentMethods.get(methodName);
        if (verifiedPaymentManifest == null) {
            verifiedPaymentManifest = new PaymentMethod();
            mVerifiedPaymentMethods.put(methodName, verifiedPaymentManifest);
        }
        return verifiedPaymentManifest;
    }

    @Override
    public void onFinishedVerification() {
        mPendingVerifiersCount--;
        if (mPendingVerifiersCount != 0) return;

        for (Map.Entry<URI, PaymentMethod> nameAndMethod : mVerifiedPaymentMethods.entrySet()) {
            URI methodName = nameAndMethod.getKey();
            if (!mUriPaymentMethods.contains(methodName)) continue;

            PaymentMethod method = nameAndMethod.getValue();
            for (ResolveInfo app : method.defaultApplications) {
                onValidPaymentAppForPaymentMethodName(app, methodName.toString());
            }

            if (method.supportsAllOrigins) {
                Set<ResolveInfo> supportedApps = mMethodToAppsMapping.get(methodName);
                for (ResolveInfo supportedApp : supportedApps) {
                    onValidPaymentAppForPaymentMethodName(supportedApp, methodName.toString());
                }
                continue;
            }

            for (URI supportedOrigin : method.supportedOrigins) {
                Set<URI> supportedAppMethodNames = mOriginToMethodsMapping.get(supportedOrigin);
                if (supportedAppMethodNames == null) continue;

                for (URI supportedAppMethodName : supportedAppMethodNames) {
                    PaymentMethod supportedAppMethod =
                            mVerifiedPaymentMethods.get(supportedAppMethodName);
                    if (supportedAppMethod == null) continue;

                    for (ResolveInfo supportedApp : supportedAppMethod.defaultApplications) {
                        onValidPaymentAppForPaymentMethodName(supportedApp, methodName.toString());
                    }
                }
            }
        }

        onAllAppsFound();
    }

    /** Notifies callback that all payment apps have been found. */
    private void onAllAppsFound() {
        assert mPendingVerifiersCount == 0;

        if (!mIsIncognito) {
            List<ResolveInfo> resolveInfos =
                    mPackageManagerDelegate.getServicesThatCanRespondToIntent(
                            new Intent(ACTION_IS_READY_TO_PAY));
            for (int i = 0; i < resolveInfos.size(); i++) {
                ResolveInfo resolveInfo = resolveInfos.get(i);
                AndroidPaymentApp app = mValidApps.get(resolveInfo.serviceInfo.packageName);
                if (app != null) app.setIsReadyToPayAction(resolveInfo.serviceInfo.name);
            }
        }

        for (Map.Entry<String, AndroidPaymentApp> entry : mValidApps.entrySet()) {
            mCallback.onPaymentAppCreated(entry.getValue());
        }

        mCallback.onAllPaymentAppsCreated();
    }

    /**
     * Enables the given payment app to use this method name.
     *
     * @param resolveInfo The payment app that's allowed to use the method name.
     * @param methodName  The method name that can be used by the app.
     */
    private void onValidPaymentAppForPaymentMethodName(ResolveInfo resolveInfo, String methodName) {
        String packageName = resolveInfo.activityInfo.packageName;
        AndroidPaymentApp app = mValidApps.get(packageName);
        if (app == null) {
            CharSequence label = mPackageManagerDelegate.getAppLabel(resolveInfo);
            if (TextUtils.isEmpty(label)) {
                Log.e(TAG, "Skipping \"%s\" because of empty label.", packageName);
                return;
            }

            app = new AndroidPaymentApp(mWebContents, packageName, resolveInfo.activityInfo.name,
                    label.toString(), mPackageManagerDelegate.getAppIcon(resolveInfo),
                    mIsIncognito);
            mValidApps.put(packageName, app);
        }

        app.addMethodName(methodName);
    }

    @Override
    public void onFinishedUsingResources() {
        mPendingResourceUsersCount--;
        if (mPendingResourceUsersCount != 0) return;

        mWebDataService.destroy();
        if (mDownloader.isInitialized()) mDownloader.destroy();
        if (mParser.isUtilityProcessRunning()) mParser.stopUtilityProcess();
    }

    @VisibleForTesting
    public static void allowHttpForTest() {
        sAllowHttpForTest = true;
    }
}