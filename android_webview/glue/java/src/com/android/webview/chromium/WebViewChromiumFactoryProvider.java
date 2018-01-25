// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.Manifest;
import android.app.ActivityManager;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Looper;
import android.os.Process;
import android.os.StrictMode;
import android.os.UserManager;
import android.provider.Settings;
import android.util.Log;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.GeolocationPermissions;
import android.webkit.ServiceWorkerController;
import android.webkit.TokenBindingService;
import android.webkit.ValueCallback;
import android.webkit.WebStorage;
import android.webkit.WebView;
import android.webkit.WebViewDatabase;
import android.webkit.WebViewFactory;
import android.webkit.WebViewFactoryProvider;
import android.webkit.WebViewProvider;

import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;

import org.chromium.android_webview.AwAutofillProvider;
import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContentsClient;
import org.chromium.android_webview.AwContentsStatics;
import org.chromium.android_webview.AwCookieManager;
import org.chromium.android_webview.AwDevToolsServer;
import org.chromium.android_webview.AwNetworkChangeNotifierRegistrationPolicy;
import org.chromium.android_webview.AwQuotaManagerBridge;
import org.chromium.android_webview.AwResource;
import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.AwSwitches;
import org.chromium.android_webview.HttpAuthDatabase;
import org.chromium.android_webview.ResourcesContextWrapperFactory;
import org.chromium.android_webview.WebViewChromiumRunQueue;
import org.chromium.android_webview.command_line.CommandLineUtil;
import org.chromium.android_webview.variations.AwVariationsSeedHandler;
import org.chromium.base.BuildConfig;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.PackageUtils;
import org.chromium.base.PathService;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.NativeLibraries;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.components.autofill.AutofillProvider;
import org.chromium.content.browser.selection.LGEmailActionModeWorkaround;
import org.chromium.net.NetworkChangeNotifier;

import java.io.File;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

/**
 * Entry point to the WebView. The system framework talks to this class to get instances of the
 * implementation classes.
 */
@SuppressWarnings("deprecation")
public class WebViewChromiumFactoryProvider implements WebViewFactoryProvider {
    private static final String TAG = "WebViewChromiumFactoryProvider";

    private static final String CHROMIUM_PREFS_NAME = "WebViewChromiumPrefs";
    private static final String VERSION_CODE_PREF = "lastVersionCodeUsed";

    // TODO make this private again after passing the run-queue to the AwHolder.
    final WebViewChromiumRunQueue mRunQueue = new WebViewChromiumRunQueue(
            () -> { return WebViewChromiumFactoryProvider.this.mAwHolder.hasStarted(); });

    // TODO could move all run-related methods into WebViewChromiumRunQueue - and then we would pass
    // that around to adapters, instead of passing WebViewChromiumFactoryProvider..
    private <T> T runBlockingFuture(FutureTask<T> task) {
        if (!mAwHolder.hasStarted()) throw new RuntimeException("Must be started before we block!");
        if (ThreadUtils.runningOnUiThread()) {
            throw new IllegalStateException("This method should only be called off the UI thread");
        }
        mRunQueue.addTask(task);
        try {
            return task.get(4, TimeUnit.SECONDS);
        } catch (java.util.concurrent.TimeoutException e) {
            throw new RuntimeException("Probable deadlock detected due to WebView API being called "
                            + "on incorrect thread while the UI thread is blocked.", e);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    // We have a 4 second timeout to try to detect deadlocks to detect and aid in debuggin
    // deadlocks.
    // Do not call this method while on the UI thread!
    /* package */ void runVoidTaskOnUiThreadBlocking(Runnable r) {
        FutureTask<Void> task = new FutureTask<Void>(r, null);
        runBlockingFuture(task);
    }

    /* package */ <T> T runOnUiThreadBlocking(Callable<T> c) {
        return runBlockingFuture(new FutureTask<T>(c));
    }

    /* package */ void addTask(Runnable task) {
        mRunQueue.addTask(task);
    }

    WebViewChromiumAwHolder mAwHolder;

    // TODO make this private again, but let AwHolder get to it somehow
    SharedPreferences mWebViewPrefs;
    private WebViewDelegate mWebViewDelegate;

    boolean mShouldDisableThreadChecking;

    /**
     * Entry point for newer versions of Android.
     */
    public static WebViewChromiumFactoryProvider create(android.webkit.WebViewDelegate delegate) {
        return new WebViewChromiumFactoryProvider(delegate);
    }

    /**
     * Constructor called by the API 21 version of {@link WebViewFactory} and earlier.
     */
    public WebViewChromiumFactoryProvider() {
        initialize(WebViewDelegateFactory.createApi21CompatibilityDelegate());
    }

    /**
     * Constructor called by the API 22 version of {@link WebViewFactory} and later.
     */
    public WebViewChromiumFactoryProvider(android.webkit.WebViewDelegate delegate) {
        initialize(WebViewDelegateFactory.createProxyDelegate(delegate));
    }

    /**
     * Constructor for internal use when a proxy delegate has already been created.
     */
    WebViewChromiumFactoryProvider(WebViewDelegate delegate) {
        initialize(delegate);
    }

    // This always runs before the WebView Support Library calls into the WebView APK code.
    private void initialize(WebViewDelegate webViewDelegate) {
        // TODO could just add a getter for WebViewDelegate from AwHolder (and remove
        // mWebViewDelegate from this class).
        mAwHolder = new WebViewChromiumAwHolder(this, webViewDelegate);
        mWebViewDelegate = webViewDelegate;
        Context ctx = mWebViewDelegate.getApplication().getApplicationContext();

        // If the application context is DE, but we have credentials, use a CE context instead
        try {
            checkStorageIsNotDeviceProtected(mWebViewDelegate.getApplication());
        } catch (IllegalArgumentException e) {
            if (!ctx.getSystemService(UserManager.class).isUserUnlocked()) {
                throw e;
            }
            ctx = ctx.createCredentialProtectedStorageContext();
        }

        // WebView needs to make sure to always use the wrapped application context.
        ContextUtils.initApplicationContext(ResourcesContextWrapperFactory.get(ctx));

        CommandLineUtil.initCommandLine();

        boolean multiProcess = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            // Ask the system if multiprocess should be enabled on O+.
            multiProcess = mWebViewDelegate.isMultiProcessEnabled();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            // Check the multiprocess developer setting directly on N.
            multiProcess = Settings.Global.getInt(
                    ContextUtils.getApplicationContext().getContentResolver(),
                    Settings.Global.WEBVIEW_MULTIPROCESS, 0) == 1;
        }
        if (multiProcess) {
            CommandLine cl = CommandLine.getInstance();
            cl.appendSwitch("webview-sandboxed-renderer");
        }

        ThreadUtils.setWillOverrideUiThread();
        // Load chromium library.
        AwBrowserProcess.loadLibrary(mWebViewDelegate.getDataDirectorySuffix());

        final PackageInfo packageInfo = WebViewFactory.getLoadedPackageInfo();

        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
        try {
            // Load glue-layer support library.
            System.loadLibrary("webviewchromium_plat_support");

            // Use shared preference to check for package downgrade.
            // Since N, getSharedPreferences creates the preference dir if it doesn't exist,
            // causing a disk write.
            mWebViewPrefs = ContextUtils.getApplicationContext().getSharedPreferences(
                    CHROMIUM_PREFS_NAME, Context.MODE_PRIVATE);
            int lastVersion = mWebViewPrefs.getInt(VERSION_CODE_PREF, 0);
            int currentVersion = packageInfo.versionCode;
            if (!versionCodeGE(currentVersion, lastVersion)) {
                // The WebView package has been downgraded since we last ran in this application.
                // Delete the WebView data directory's contents.
                String dataDir = PathUtils.getDataDirectory();
                Log.i(TAG, "WebView package downgraded from " + lastVersion
                        + " to " + currentVersion + "; deleting contents of " + dataDir);
                deleteContents(new File(dataDir));
            }
            if (lastVersion != currentVersion) {
                mWebViewPrefs.edit().putInt(VERSION_CODE_PREF, currentVersion).apply();
            }
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        mShouldDisableThreadChecking =
                shouldDisableThreadChecking(ContextUtils.getApplicationContext());
        // Now safe to use WebView data directory.
    }

    static void checkStorageIsNotDeviceProtected(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N && context.isDeviceProtectedStorage()) {
            throw new IllegalArgumentException(
                    "WebView cannot be used with device protected storage");
        }
    }

    /**
     * Both versionCodes should be from a WebView provider package implemented by Chromium.
     * VersionCodes from other kinds of packages won't make any sense in this method.
     *
     * An introduction to Chromium versionCode scheme:
     * "BBBBPPPAX"
     * BBBB: 4 digit branch number. It monotonically increases over time.
     * PPP: patch number in the branch. It is padded with zeroes to the left. These three digits may
     * change their meaning in the future.
     * A: architecture digit.
     * X: A digit to differentiate APKs for other reasons.
     *
     * This method takes the "BBBB" of versionCodes and compare them.
     *
     * @return true if versionCode1 is higher than or equal to versionCode2.
     */
    private static boolean versionCodeGE(int versionCode1, int versionCode2) {
        int v1 = versionCode1 / 100000;
        int v2 = versionCode2 / 100000;

        return v1 >= v2;
    }

    private static void deleteContents(File dir) {
        File[] files = dir.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isDirectory()) {
                    deleteContents(file);
                }
                if (!file.delete()) {
                    Log.w(TAG, "Failed to delete " + file);
                }
            }
        }
    }

    public static boolean preloadInZygote() {
        for (String library : NativeLibraries.LIBRARIES) {
            System.loadLibrary(library);
        }
        return true;
    }

    // TODO how do we deal with Statics? We can't share the Statics object between the two glue
    // layers.. so how will the support library glue deal with Statics?
    @Override
    public Statics getStatics() {
        return mAwHolder.getStatics();
    }

    @Override
    public WebViewProvider createWebView(WebView webView, WebView.PrivateAccess privateAccess) {
        return new WebViewChromium(this, webView, privateAccess, mShouldDisableThreadChecking);
    }

    // Workaround for IME thread crashes on grandfathered OEM apps.
    private boolean shouldDisableThreadChecking(Context context) {
        String appName = context.getPackageName();
        int versionCode = PackageUtils.getPackageVersion(context, appName);
        int appTargetSdkVersion = context.getApplicationInfo().targetSdkVersion;
        if (versionCode == -1) return false;

        boolean shouldDisable = false;

        // crbug.com/651706
        final String lgeMailPackageId = "com.lge.email";
        if (lgeMailPackageId.equals(appName)) {
            if (appTargetSdkVersion > Build.VERSION_CODES.N) return false;
            // This is the last broken version shipped on LG V20/NRD90M.
            if (versionCode > LGEmailActionModeWorkaround.LGEmailWorkaroundMaxVersion) return false;
            shouldDisable = true;
        }

        // crbug.com/655759
        // Also want to cover ".att" variant suffix package name.
        final String yahooMailPackageId = "com.yahoo.mobile.client.android.mail";
        if (appName.startsWith(yahooMailPackageId)) {
            if (appTargetSdkVersion > Build.VERSION_CODES.M) return false;
            if (versionCode > 1315850) return false;
            shouldDisable = true;
        }

        // crbug.com/622151
        final String htcMailPackageId = "com.htc.android.mail";
        if (htcMailPackageId.equals(appName)) {
            if (appTargetSdkVersion > Build.VERSION_CODES.M) return false;
            // This value is provided by HTC.
            if (versionCode >= 866001861) return false;
            shouldDisable = true;
        }

        if (shouldDisable) {
            Log.w(TAG, "Disabling thread check in WebView. "
                            + "APK name: " + appName + ", versionCode: " + versionCode
                            + ", targetSdkVersion: " + appTargetSdkVersion);
        }
        return shouldDisable;
    }

    @Override
    public GeolocationPermissions getGeolocationPermissions() {
        return mAwHolder.getGeolocationPermissions();
    }

    @Override
    public CookieManager getCookieManager() {
        return mAwHolder.getCookieManager();
    }

    @Override
    public ServiceWorkerController getServiceWorkerController() {
        return mAwHolder.getServiceWorkerController();
    }

    @Override
    public TokenBindingService getTokenBindingService() {
        return mAwHolder.getTokenBindingService();
    }

    @Override
    public android.webkit.WebIconDatabase getWebIconDatabase() {
        return mAwHolder.getWebIconDatabase();
    }

    @Override
    public WebStorage getWebStorage() {
        return mAwHolder.getWebStorage();
    }

    @Override
    public WebViewDatabase getWebViewDatabase(final Context context) {
        // TODO return new WebViewDatabaseAdapter(mAwHolder.getWebViewDatabase(context));
        return mAwHolder.getWebViewDatabase(context);
    }

    WebViewDelegate getWebViewDelegate() {
        return mWebViewDelegate;
    }

    WebViewContentsClientAdapter createWebViewContentsClientAdapter(WebView webView,
            Context context) {
        return new WebViewContentsClientAdapter(webView, context, mWebViewDelegate);
    }

    AutofillProvider createAutofillProvider(Context context, ViewGroup containerView) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return null;
        return new AwAutofillProvider(context, containerView);
    }

    public WebViewChromiumAwHolder getAwHolder() {
        return mAwHolder;
    }

    void startYourEngines(boolean onMainThread) {
        mAwHolder.startYourEngines(onMainThread);
    }

    boolean hasStarted() {
        return mAwHolder.hasStarted();
    }

    // TODO this is only used from WebViewChromium?
    // Only on UI thread.
    AwBrowserContext getBrowserContextOnUiThread() {
        return mAwHolder.getBrowserContextOnUiThread();
    }

    // TODO this is needed for overriding by internal WebViewChromiumFactoryProviderForP
    // TODO this is ugly... we might call WebViewChromiumAwHolder without calling into
    // WebViewChromiumFactoryProvider...
    protected void startChromiumLocked() {
        mAwHolder.startChromiumLocked();
    }
}
