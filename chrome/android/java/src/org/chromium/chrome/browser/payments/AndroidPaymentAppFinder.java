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
    private final Map<String, AndroidPaymentApp> mValidApps = new HashMap<>();

    /**
     * A mapping from origins of payment apps to the URI payment methods of these apps.
     */
     private final Map<URI, Set<URI>> mPaymentAppOriginsAndUriPaymentMethods = new HashMap<>();

    /**
     * A mapping from URI payment methods to the applications that support this payment method.
     */
    private final Map<URI, Set<ResolveInfo>> mPaymentMethodsAndPaymentApps = new HashMap<>();

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
            onAllAppsFound();
            return;
        }

        Set<URI> allUriPaymentMethods = new HashSet<>();
        allUriPaymentMethods.addAll(mUriPaymentMethods);

        List<Set<String>> appDefaultSupportedMethods = new ArrayList<>();
        List<Set<String>> appNonDefaultSupportedMethods = new ArrayList<>();
        List<Set<String>> appAllSupportedMethods = new ArrayList<>();
        Map<URI, Set<URI>> nonDefaultAppOrigins = new HashMap<>();

        for (int i = 0; i < apps.size(); i++) {
            ResolveInfo app = apps.get(i);
            Set<String> defaultMethods = getDefaultPaymentMethodName(app.activityInfo);
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

            Set<String> nonDefaultMethods = getNonDefaultPaymentMethodNames(app.activityInfo);
            appNonDefaultSupportedMethods.add(nonDefaultMethods);

            Set<String> allMethods = new HashSet<>();
            allMethods.addAll(defaultMethods);
            allMethods.addAll(nonDefaultMethods);
            appAllSupportedMethods.add(allMethods);

            Set<URI> supportedUriPaymentMethods = new HashSet<>();
            for (String method : nonDefaultMethods) {
                if (isUriMethod(method)) {
                    URI uriMethod = parseAbsoluteUriMethodNameFromString(method);
                    if (uriMethod != null) {
                        supportedUriPaymentMethods.add(uriMethod);
                        if (appDefaultUriMethodName != null) {
                            Set<URI> origins = nonDefaultAppOrigins.get(uriMethod);
                            if (origins == null) {
                                origins = new HashSet<>();
                                nonDefaultAppOrigins.put(uriMethod, origins);
                            }
                            origins.add(getOriginOfPaymentMethodName(appDefaultUriMethodName));
                        }
                    }
                }
            }

            if (appDefaultUriMethodName != null) {
                URI appOrigin = getOriginOfPaymentMethodName(appDefaultUriMethodName);
                Set<URI> methods = mPaymentAppOriginsAndUriPaymentMethods.get(appOrigin);
                if (methods == null) {
                    methods = new HashSet<>();
                    mPaymentAppOriginsAndUriPaymentMethods.put(appOrigin, methods);
                }
                Log.e(TAG, "mPaymentAppOriginsAndUriPaymentMethods.get(%s).add(%s);", appOrigin, appDefaultUriMethodName);
                methods.add(appDefaultUriMethodName);
            }

            for (URI paymentMethod : supportedUriPaymentMethods) {
                Set<ResolveInfo> paymentAppDefaultMethods =
                        mPaymentMethodsAndPaymentApps.get(paymentMethod);
                if (paymentAppDefaultMethods == null) {
                    paymentAppDefaultMethods = new HashSet<>();
                    mPaymentMethodsAndPaymentApps.put(paymentMethod, paymentAppDefaultMethods);
                }
                paymentAppDefaultMethods.add(app);
            }
        }

        List<PaymentManifestVerifier> verifiers = new ArrayList<>();
        for (URI methodName : allUriPaymentMethods) {
            if (filterAppsByMethodName(
                    apps, appAllSupportedMethods, methodName.toString()).isEmpty()) {
                Log.e(TAG, "no apps for '%s'", methodName);
                continue;
            }
            Log.e(TAG, "found apps for '%s'", methodName);

            // Start the parser utility process as soon as possible, once we know that a manifest
            // file needs to be parsed. The startup can take up to 2 seconds.
            if (!mParser.isUtilityProcessRunning()) mParser.startUtilityProcess();

            // Initialize the native side of the downloader, once we know that a manifest file needs
            // to be downloaded.
            if (!mDownloader.isInitialized()) mDownloader.initialize(mWebContents);

            verifiers.add(new PaymentManifestVerifier(
                    methodName, new HashSet<>(filterAppsByMethodName(
                    apps, appDefaultSupportedMethods, methodName.toString())),
                    nonDefaultAppOrigins.get(methodName), mWebDataService,
                    mDownloader, mParser, mPackageManagerDelegate, this /* callback */));

            if (verifiers.size() == MAX_NUMBER_OF_MANIFESTS) {
                Log.e(TAG, "Reached maximum number of allowed payment app manifests.");
                break;
            }
        }

        for (String nonUriMethodName : mNonUriPaymentMethods) {
            List<ResolveInfo> supportedApps =
                    filterAppsByMethodName(apps, appAllSupportedMethods, nonUriMethodName);
            for (int i = 0; i < supportedApps.size(); i++) {
                // Chrome does not verify app manifests for non-URI payment method support.
                onValidPaymentAppForPaymentMethodName(supportedApps.get(i), nonUriMethodName);
            }
        }

        if (verifiers.isEmpty()) {
            onAllAppsFound();
            return;
        }

        mPendingVerifiersCount = mPendingResourceUsersCount = verifiers.size();
        for (PaymentManifestVerifier verifier : verifiers) {
            verifier.verify();
        }
    }

    private static URI getOriginOfPaymentMethodName(URI paymentMethodName) {
        // Tests use sub-directories to simulate different hosts, because the test web server
        // runs on a single localhost origin. Therefore, the "origin" in test is
        // https://127.0.0.1:12355/components/test/data/payments/bobpay.xyz instead of
        // https://127.0.0.1:12355.
        URI result = parseAbsoluteUriMethodNameFromString(removeTrailingSlash(
                paymentMethodName.resolve(sAllowHttpForTest ? "." : "/").toString()));
        assert result != null;
        return result;
    }

    private static String removeTrailingSlash(String path) {
        return path.isEmpty() || path.charAt(path.length() - 1) != '/' ?  path : path.substring
                (0, path.length() - 1);
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

    private static List<ResolveInfo> filterAppsByMethodName(
            List<ResolveInfo> apps, List<Set<String>> methodNames, String targetMethodName) {
        assert apps.size() == methodNames.size();
        List<ResolveInfo> supportedApps = new ArrayList<>();

        // Note that apps and  methodNames must have the same size. The information at the same
        // index must correspond to the same app.
        for (int i = 0; i < apps.size(); i++) {
            if (methodNames.get(i).contains(targetMethodName)) {
                supportedApps.add(apps.get(i));
                continue;
            }
        }
        return supportedApps;
    }

    private PaymentMethod getVerifiedPaymentMethod(URI methodName) {
        PaymentMethod verifiedPaymentManifest = mVerifiedPaymentMethods.get(methodName);
        if (verifiedPaymentManifest == null) {
            verifiedPaymentManifest = new PaymentMethod();
            mVerifiedPaymentMethods.put(methodName, verifiedPaymentManifest);
        }
        return verifiedPaymentManifest;
    }

    @Override
    public void onValidDefaultPaymentApp(URI methodName, ResolveInfo resolveInfo) {
        Log.e(TAG, "%s is a valid default app for %s", resolveInfo.activityInfo.packageName,
                methodName);
        getVerifiedPaymentMethod(methodName).defaultApplications.add(resolveInfo);
    }

    @Override
    public void onValidSupportedOrigin(URI methodName, URI supportedOrigin) {
        Log.e(TAG, "%s supports payment apps from origin %s", methodName, supportedOrigin);
        getVerifiedPaymentMethod(methodName).supportedOrigins.add(supportedOrigin);
    }

    @Override
    public void onAllOriginsSupported(URI methodName) {
        Log.e(TAG, "%s supports payment apps from all origins", methodName);
        getVerifiedPaymentMethod(methodName).supportsAllOrigins = true;
    }

    @Override
    public void onFinishedVerification() {
        mPendingVerifiersCount--;
        Log.e(TAG, "One verifier finished. %d remaining.", mPendingVerifiersCount);
        if (mPendingVerifiersCount != 0) return;

        for (Map.Entry<URI, PaymentMethod> nameAndMethod : mVerifiedPaymentMethods.entrySet()) {
            URI methodName = nameAndMethod.getKey();
            if (!mUriPaymentMethods.contains(methodName)) continue;

            PaymentMethod method = nameAndMethod.getValue();
            for (ResolveInfo app : method.defaultApplications) {
                Log.e(TAG, "%s has default application %s", methodName,
                        app.activityInfo.packageName);
                onValidPaymentAppForPaymentMethodName(app, methodName.toString());
            }

            if (method.supportsAllOrigins) {
                Log.e(TAG, "%s supports all origins", methodName);
                Set<ResolveInfo> supportedApps
                        = mPaymentMethodsAndPaymentApps.get(methodName);
                for (ResolveInfo supportedApp : supportedApps) {
                    onValidPaymentAppForPaymentMethodName(supportedApp, methodName.toString());
                }
                continue;
            }

            for (URI supportedOrigin : method.supportedOrigins) {
                Log.e(TAG, "%s supports origin %s", methodName, supportedOrigin);
                Set<URI> supportedAppMethodNames =
                        mPaymentAppOriginsAndUriPaymentMethods.get(supportedOrigin);
                if (supportedAppMethodNames == null) {
                    Log.e(TAG, "mPaymentAppOriginsAndUriPaymentMethods.get(%s) == null",
                            supportedOrigin);
                    continue;
                }

                Log.e(TAG, "mPaymentAppOriginsAndSupportedUriPaymentMethods.get(%s).size() == %d",
                        supportedOrigin, supportedAppMethodNames.size());

                for (URI supportedAppMethodName : supportedAppMethodNames) {
                    PaymentMethod supportedAppMethod = mVerifiedPaymentMethods.get(supportedAppMethodName);
                    if (supportedAppMethod == null) {
                        Log.e(TAG, "Did not find any valid apps for payment method name %s",
                                supportedAppMethodName);
                        continue;
                    }

                    Log.e(TAG, "Found %d valid apps for default payment method name %s",
                            supportedAppMethod.defaultApplications.size(), supportedAppMethodName);

                    for (ResolveInfo supportedApp : supportedAppMethod.defaultApplications) {
                        onValidPaymentAppForPaymentMethodName(supportedApp, methodName.toString());
                    }
                }
            }
        }

        onAllAppsFound();
    }

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

    private void onValidPaymentAppForPaymentMethodName(ResolveInfo resolveInfo, String methodName) {
        String packageName = resolveInfo.activityInfo.packageName;
        AndroidPaymentApp app = mValidApps.get(packageName);
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
            mValidApps.put(packageName, app);
        }
        app.addMethodName(methodName);
    }

    @Override
    public void onFinishedUsingResources() {
        mPendingResourceUsersCount--;
        Log.e(TAG, "One resource user finished. %d remaining.", mPendingResourceUsersCount);
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