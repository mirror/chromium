// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.Manifest;
import android.app.ActivityManager;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Looper;
import android.os.Process;
import android.util.Log;
import android.webkit.CookieManager;
import android.webkit.GeolocationPermissions;
import android.webkit.ServiceWorkerController;
import android.webkit.TokenBindingService;
import android.webkit.ValueCallback;
import android.webkit.WebStorage;
import android.webkit.WebViewDatabase;
import android.webkit.WebViewFactory;
import android.webkit.WebViewFactoryProvider;
import android.webkit.WebViewFactoryProvider.Statics;

import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;

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
import org.chromium.android_webview.AwTracingController;
import org.chromium.android_webview.HttpAuthDatabase;
import org.chromium.android_webview.VariationsUtils;
import org.chromium.android_webview.command_line.CommandLineUtil;
import org.chromium.base.BuildConfig;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.PathService;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.components.variations.firstrun.VariationsSeedBridge;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;
import org.chromium.net.NetworkChangeNotifier;

import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

class WebViewChromiumAwInit {

    private static final String TAG = "WebViewChromiumAwInit";

    private static final String HTTP_AUTH_DATABASE_FILE = "http_auth.db";
    private static final long SEED_LOAD_TIMEOUT_MILLIS = 20;

    // SeedLoaderTask wraps a FutureTask to asynchronously load the variations seed. The steps are:
    // 1. Load the new seed file, if any.
    // 2. If no new seed file, load the old seed file, if any.
    // 3. Make the loaded seed available via get() (or null if there was no seed).
    // 4. If there was a new seed file, replace the old with the new (but only after making the
    //    loaded seed available, as the replace need not block startup).
    // 5. If there was no seed, or the loaded seed was expired, request a new seed (but don't
    //    request more often than MAX_REQUEST_PERIOD_MILLIS).
    private static class SeedLoaderTask implements Runnable {
        // The expiration time for an app's copy of the Finch seed, after which we'll still use it,
        // but we'll request a new one from the Service.
        private static final long SEED_EXPIRATION_MILLIS = TimeUnit.HOURS.toMillis(6);

        // After requesting a new seed, wait at least this long before making a new request.
        private static final long MAX_REQUEST_PERIOD_MILLIS = TimeUnit.MINUTES.toMillis(10);

        private static boolean isExpired(long seedFileTime) {
            long expirationTime = seedFileTime + SEED_EXPIRATION_MILLIS;
            long now = (new Date()).getTime();
            return now > expirationTime;
        }

        // Is there a new seed file present (which should replace the old seed file)?
        private boolean mFoundNewSeed;
        // Should we request a new seed from the service?
        private boolean mNeedNewSeed;

        private FutureTask<SeedInfo> mTask = new FutureTask<SeedInfo>(new Callable<SeedInfo>() {
            @Override
            public SeedInfo call() {
                File newSeedFile = VariationsUtils.getNewSeedFile();
                File oldSeedFile = VariationsUtils.getSeedFile();

                // The time (in milliseconds since epoch) the seed was last written.
                long seedFileTime = 0;

                // First check for a new seed.
                SeedInfo seed = VariationsUtils.readSeedFile(newSeedFile);
                if (seed != null) {
                    // If a valid new seed was found, make a note to replace the old seed with
                    // the new seed. (Don't do it now, to avaid delaying FutureTask.get().)
                    mFoundNewSeed = true;

                    seedFileTime = newSeedFile.lastModified();
                } else {
                    // If there is no new seed, check for an old seed.
                    seed = VariationsUtils.readSeedFile(oldSeedFile);

                    if (seed != null) {
                        seedFileTime = oldSeedFile.lastModified();
                    }
                }

                // Make a note to request a new seed if necessary. (Don't request it now, to
                // avaid delaying FutureTask.get().)
                mNeedNewSeed = (seed == null || isExpired(seedFileTime));

                return seed;
            }
        });

        @Override
        public void run() {
            mTask.run();

            // The loaded seed is now available via get(). Tho following steps won't block startup.

            if (mFoundNewSeed) {
                // This happens synchronously.
                VariationsUtils.replaceOldWithNewSeed();
            }

            if (mNeedNewSeed) {
                // Rate-limit seed requests using an extra "stamp" file whose modification time is
                // the time of the last request.
                File seedRequestStamp =
                        new File(PathUtils.getDataDirectory(), "variations_seed_stamp");

                long now = (new Date()).getTime();
                long lastRequestTime = seedRequestStamp.lastModified(); // 0 if file not found.
                if (lastRequestTime != 0 && now < lastRequestTime + MAX_REQUEST_PERIOD_MILLIS)
                    return;

                try {
                    if (!seedRequestStamp.createNewFile()) {
                        // If the file already exists, update the time stamp.
                        seedRequestStamp.setLastModified(now);
                    }
                } catch (IOException e) {
                    Log.e(TAG, "Failed to write " + seedRequestStamp);
                    return;
                }

                // The new seed will arrive asynchronously; the new seed file is written by the
                // service, and may complete after this app process has died.
                VariationsUtils.requestSeedFromService();
            }
        }

        public SeedInfo get(long timeout, TimeUnit unit)
                throws ExecutionException, InterruptedException, TimeoutException {
            return mTask.get(timeout, unit);
        }
    };

    // TODO(gsennton): store aw-objects instead of adapters here
    // Initialization guarded by mLock.
    private AwBrowserContext mBrowserContext;
    private Statics mStaticMethods;
    private GeolocationPermissionsAdapter mGeolocationPermissions;
    private CookieManagerAdapter mCookieManager;
    private Object mTokenBindingManager;
    private WebIconDatabaseAdapter mWebIconDatabase;
    private WebStorageAdapter mWebStorage;
    private WebViewDatabaseAdapter mWebViewDatabase;
    private AwDevToolsServer mDevToolsServer;
    private Object mServiceWorkerController;

    // Guards accees to the other members, and is notifyAll() signalled on the UI thread
    // when the chromium process has been started.
    // This member is not private only because the downstream subclass needs to access it,
    // it shouldn't be accessed from anywhere else.
    /* package */ final Object mLock = new Object();

    // Read/write protected by mLock.
    private boolean mStarted;

    private final WebViewChromiumFactoryProvider mFactory;

    WebViewChromiumAwInit(WebViewChromiumFactoryProvider factory) {
        mFactory = factory;
        // Do not make calls into 'factory' in this ctor - this ctor is called from the
        // WebViewChromiumFactoryProvider ctor, so 'factory' is not properly initialized yet.
    }

    AwTracingController getTracingControllerOnUiThread() {
        synchronized (mLock) {
            ensureChromiumStartedLocked(true);
            return getBrowserContextOnUiThread().getTracingController();
        }
    }

    // TODO: DIR_RESOURCE_PAKS_ANDROID needs to live somewhere sensible,
    // inlined here for simplicity setting up the HTMLViewer demo. Unfortunately
    // it can't go into base.PathService, as the native constant it refers to
    // lives in the ui/ layer. See ui/base/ui_base_paths.h
    private static final int DIR_RESOURCE_PAKS_ANDROID = 3003;

    protected void startChromiumLocked() {
        assert Thread.holdsLock(mLock) && ThreadUtils.runningOnUiThread();

        // The post-condition of this method is everything is ready, so notify now to cover all
        // return paths. (Other threads will not wake-up until we release |mLock|, whatever).
        mLock.notifyAll();

        if (mStarted) {
            return;
        }

        // The WebView package name must be set early because:
        // - It's used to locate the service to which we copy crash minidumps. The name must be set
        //   before a render process has a chance to crash - otherwise we might try to copy a
        //   minidump without knowing what process to copy it to.
        // - It's used to locate the service from which we get the variations seed, so it must be
        //   set before initializing variations.
        // - It's used to determine channel for UMA, so it must be set before initializing UMA.
        final PackageInfo webViewPackageInfo = WebViewFactory.getLoadedPackageInfo();
        final String webViewPackageName = webViewPackageInfo.packageName;
        AwBrowserProcess.setWebViewPackageName(webViewPackageName);

        // TODO(paulmiller) AGSA switch
        boolean enableVariations =
                CommandLine.getInstance().hasSwitch(AwSwitches.ENABLE_WEBVIEW_VARIATIONS);
        Log.d(TAG, "enableVariations=" + enableVariations);
        SeedLoaderTask seedLoaderTask = null;
        if (enableVariations) {
            // Begin asynchronously loading the variations seed.
            seedLoaderTask = new SeedLoaderTask();
            (new Thread(seedLoaderTask)).start();
        }

        try {
            LibraryLoader.get(LibraryProcessType.PROCESS_WEBVIEW).ensureInitialized();
        } catch (ProcessInitException e) {
            throw new RuntimeException("Error initializing WebView library", e);
        }

        PathService.override(PathService.DIR_MODULE, "/system/lib/");
        PathService.override(DIR_RESOURCE_PAKS_ANDROID, "/system/framework/webview/paks");

        // Make sure that ResourceProvider is initialized before starting the browser process.
        final Context context = ContextUtils.getApplicationContext();
        setUpResources(webViewPackageInfo, context);
        initPlatSupportLibrary();
        doNetworkInitializations(context);
        final boolean isExternalService = true;
        AwBrowserProcess.configureChildProcessLauncher(webViewPackageName, isExternalService);
        AwBrowserProcess.start();
        AwBrowserProcess.handleMinidumpsAndSetMetricsConsent(
                webViewPackageName, true /* updateMetricsConsent */);

        if (CommandLineUtil.isBuildDebuggable()) {
            setWebContentsDebuggingEnabled(true);
        }

        TraceEvent.setATraceEnabled(mFactory.getWebViewDelegate().isTraceTagEnabled());
        mFactory.getWebViewDelegate().setOnTraceEnabledChangeListener(
                new WebViewDelegate.OnTraceEnabledChangeListener() {
                    @Override
                    public void onTraceEnabledChange(boolean enabled) {
                        TraceEvent.setATraceEnabled(enabled);
                    }
                });

        mStarted = true;

        // Initialize thread-unsafe singletons.
        AwBrowserContext awBrowserContext = getBrowserContextOnUiThread();
        mGeolocationPermissions = new GeolocationPermissionsAdapter(
                mFactory, awBrowserContext.getGeolocationPermissions());
        mWebStorage = new WebStorageAdapter(mFactory, AwQuotaManagerBridge.getInstance());
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
            mServiceWorkerController = new ServiceWorkerControllerAdapter(
                    awBrowserContext.getServiceWorkerController());
        }

        mFactory.getRunQueue().drainQueue();

        if (enableVariations) {
            try {
                SeedInfo seed = seedLoaderTask.get(SEED_LOAD_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
                if (seed != null) {
                    VariationsSeedBridge.setVariationsFirstRunSeed(seed.seedData, seed.signature,
                            seed.country, seed.date, seed.isGzipCompressed);
                }
            } catch (TimeoutException e) {
                // TODO(paulmiller): Log seed load time and success rate in UMA.
                Log.w(TAG, "Timeout out waiting for variations seed - variations disabled");
            } catch (InterruptedException | ExecutionException e) {
                Log.e(TAG, "Failed loading variations seed - variations disabled", e);
            }
        }
    }

    private void setUpResources(PackageInfo webViewPackageInfo, Context context) {
        String packageName = webViewPackageInfo.packageName;
        if (webViewPackageInfo.applicationInfo.metaData != null) {
            packageName = webViewPackageInfo.applicationInfo.metaData.getString(
                    "com.android.webview.WebViewDonorPackage", packageName);
        }
        ResourceRewriter.rewriteRValues(
                mFactory.getWebViewDelegate().getPackageId(context.getResources(), packageName));

        AwResource.setResources(context.getResources());
        AwResource.setConfigKeySystemUuidMapping(android.R.array.config_keySystemUuidMapping);
    }

    boolean hasStarted() {
        return mStarted;
    }

    void startYourEngines(boolean onMainThread) {
        synchronized (mLock) {
            ensureChromiumStartedLocked(onMainThread);
        }
    }

    // This method is not private only because the downstream subclass needs to access it,
    // it shouldn't be accessed from anywhere else.
    /* package */ void ensureChromiumStartedLocked(boolean onMainThread) {
        assert Thread.holdsLock(mLock);

        if (mStarted) { // Early-out for the common case.
            return;
        }

        Looper looper = !onMainThread ? Looper.myLooper() : Looper.getMainLooper();
        Log.v(TAG, "Binding Chromium to "
                        + (Looper.getMainLooper().equals(looper) ? "main" : "background")
                        + " looper " + looper);
        ThreadUtils.setUiThread(looper);

        if (ThreadUtils.runningOnUiThread()) {
            startChromiumLocked();
            return;
        }

        // We must post to the UI thread to cover the case that the user has invoked Chromium
        // startup by using the (thread-safe) CookieManager rather than creating a WebView.
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                synchronized (mLock) {
                    startChromiumLocked();
                }
            }
        });
        while (!mStarted) {
            try {
                // Important: wait() releases |mLock| the UI thread can take it :-)
                mLock.wait();
            } catch (InterruptedException e) {
                // Keep trying... eventually the UI thread will process the task we sent it.
            }
        }
    }

    private void initPlatSupportLibrary() {
        DrawGLFunctor.setChromiumAwDrawGLFunction(AwContents.getAwDrawGLFunction());
        AwContents.setAwDrawSWFunctionTable(GraphicsUtils.getDrawSWFunctionTable());
        AwContents.setAwDrawGLFunctionTable(GraphicsUtils.getDrawGLFunctionTable());
    }

    private void doNetworkInitializations(Context applicationContext) {
        if (applicationContext.checkPermission(Manifest.permission.ACCESS_NETWORK_STATE,
                Process.myPid(), Process.myUid()) == PackageManager.PERMISSION_GRANTED) {
            NetworkChangeNotifier.init();
            NetworkChangeNotifier.setAutoDetectConnectivityState(
                    new AwNetworkChangeNotifierRegistrationPolicy());
        }

        AwContentsStatics.setCheckClearTextPermitted(
                applicationContext.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.O);
    }

    // Only on UI thread.
    AwBrowserContext getBrowserContextOnUiThread() {
        assert mStarted;

        if (BuildConfig.DCHECK_IS_ON && !ThreadUtils.runningOnUiThread()) {
            throw new RuntimeException(
                    "getBrowserContextOnUiThread called on " + Thread.currentThread());
        }

        if (mBrowserContext == null) {
            mBrowserContext = new AwBrowserContext(
                mFactory.getWebViewPrefs(), ContextUtils.getApplicationContext());
        }
        return mBrowserContext;
    }

    // TODO(gsennton) create an AwStatics class to return from this class
    public Statics getStatics() {
        synchronized (mLock) {
            if (mStaticMethods == null) {
                // TODO: Optimization potential: most these methods only need the native library
                // loaded and initialized, not the entire browser process started.
                // See also http://b/7009882
                ensureChromiumStartedLocked(true);
                mStaticMethods = new WebViewFactoryProvider.Statics() {
                    @Override
                    public String findAddress(String addr) {
                        return AwContentsStatics.findAddress(addr);
                    }

                    @Override
                    public String getDefaultUserAgent(Context context) {
                        return AwSettings.getDefaultUserAgent();
                    }

                    @Override
                    public void setWebContentsDebuggingEnabled(boolean enable) {
                        // Web Contents debugging is always enabled on debug builds.
                        if (!CommandLineUtil.isBuildDebuggable()) {
                            WebViewChromiumAwInit.this.setWebContentsDebuggingEnabled(
                                    enable);
                        }
                    }

                    @Override
                    public void clearClientCertPreferences(Runnable onCleared) {
                        // clang-format off
                        ThreadUtils.runOnUiThread(() ->
                                AwContentsStatics.clearClientCertPreferences(onCleared));
                        // clang-format on
                    }

                    @Override
                    public void freeMemoryForTests() {
                        if (ActivityManager.isRunningInTestHarness()) {
                            MemoryPressureListener.maybeNotifyMemoryPresure(
                                    ComponentCallbacks2.TRIM_MEMORY_COMPLETE);
                        }
                    }

                    @Override
                    public void enableSlowWholeDocumentDraw() {
                        WebViewChromium.enableSlowWholeDocumentDraw();
                    }

                    @Override
                    public Uri[] parseFileChooserResult(int resultCode, Intent intent) {
                        return AwContentsClient.parseFileChooserResult(resultCode, intent);
                    }

                    /**
                     * Starts Safe Browsing initialization. This should only be called once.
                     * @param context is the application context the WebView will be used in.
                     * @param callback will be called with the value true if initialization is
                     * successful. The callback will be run on the UI thread.
                     */
                    @Override
                    public void initSafeBrowsing(Context context, ValueCallback<Boolean> callback) {
                        // clang-format off
                        ThreadUtils.runOnUiThread(() -> AwContentsStatics.initSafeBrowsing(context,
                                    CallbackConverter.fromValueCallback(callback)));
                        // clang-format on
                    }

                    @Override
                    public void setSafeBrowsingWhitelist(
                            List<String> urls, ValueCallback<Boolean> callback) {
                        // clang-format off
                        ThreadUtils.runOnUiThread(() -> AwContentsStatics.setSafeBrowsingWhitelist(
                                urls, CallbackConverter.fromValueCallback(callback)));
                        // clang-format on
                    }

                    /**
                     * Returns a URL pointing to the privacy policy for Safe Browsing reporting.
                     *
                     * @return the url pointing to a privacy policy document which can be displayed
                     * to users.
                     */
                    @Override
                    public Uri getSafeBrowsingPrivacyPolicyUrl() {
                        return AwContentsStatics.getSafeBrowsingPrivacyPolicyUrl();
                    }
                };
            }
        }
        return mStaticMethods;
    }

    private void setWebContentsDebuggingEnabled(boolean enable) {
        if (Looper.myLooper() != ThreadUtils.getUiThreadLooper()) {
            throw new RuntimeException(
                    "Toggling of Web Contents Debugging must be done on the UI thread");
        }
        if (mDevToolsServer == null) {
            if (!enable) return;
            mDevToolsServer = new AwDevToolsServer();
        }
        mDevToolsServer.setRemoteDebuggingEnabled(enable);
    }

    public GeolocationPermissions getGeolocationPermissions() {
        synchronized (mLock) {
            if (mGeolocationPermissions == null) {
                ensureChromiumStartedLocked(true);
            }
        }
        return mGeolocationPermissions;
    }

    public CookieManager getCookieManager() {
        synchronized (mLock) {
            if (mCookieManager == null) {
                mCookieManager = new CookieManagerAdapter(new AwCookieManager());
            }
        }
        return mCookieManager;
    }

    public ServiceWorkerController getServiceWorkerController() {
        synchronized (mLock) {
            if (mServiceWorkerController == null) {
                ensureChromiumStartedLocked(true);
            }
        }
        return (ServiceWorkerController) mServiceWorkerController;
    }

    public TokenBindingService getTokenBindingService() {
        synchronized (mLock) {
            if (mTokenBindingManager == null) {
                mTokenBindingManager = new TokenBindingManagerAdapter(mFactory);
            }
        }
        return (TokenBindingService) mTokenBindingManager;
    }

    public android.webkit.WebIconDatabase getWebIconDatabase() {
        synchronized (mLock) {
            if (mWebIconDatabase == null) {
                ensureChromiumStartedLocked(true);
                mWebIconDatabase = new WebIconDatabaseAdapter();
            }
        }
        return mWebIconDatabase;
    }

    public WebStorage getWebStorage() {
        synchronized (mLock) {
            if (mWebStorage == null) {
                ensureChromiumStartedLocked(true);
            }
        }
        return mWebStorage;
    }

    public WebViewDatabase getWebViewDatabase(final Context context) {
        synchronized (mLock) {
            if (mWebViewDatabase == null) {
                ensureChromiumStartedLocked(true);
                mWebViewDatabase = new WebViewDatabaseAdapter(
                        mFactory, HttpAuthDatabase.newInstance(context, HTTP_AUTH_DATABASE_FILE));
            }
        }
        return mWebViewDatabase;
    }
}
