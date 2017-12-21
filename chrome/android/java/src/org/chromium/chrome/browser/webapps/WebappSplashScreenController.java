// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.StreamUtil;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.metrics.WebappUma;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.webapps.WebappActivity.ActivityType;
import org.chromium.content_public.browser.ContentBitmapCallback;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.NetError;
import org.chromium.net.NetworkChangeNotifier;
import org.chromium.ui.UiUtils;
import org.chromium.ui.base.PageTransition;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/** Shows and hides splash screen. */
class WebappSplashScreenController extends EmptyTabObserver {
    private static final String TAG = "WebappSplashScreen";

    /** Used to schedule splash screen hiding. */
    private CompositorViewHolder mCompositorViewHolder;

    /** View to which the splash screen is added. */
    private ViewGroup mParentView;

    /** Whether native was loaded. Native must be loaded in order to record metrics. */
    private boolean mNativeLoaded;

    /** Whether the splash screen layout was initialized. */
    private boolean mInitializedLayout;

    private WebappActivity mWebappActivity;
    private ViewGroup mSplashScreen;
    private WebappUma mWebappUma;

    /** Whether we are in the process of navigating to and screenshotting the splash_screen_url */
    private boolean mInSplashScreenCapturePhase;

    /** The error code of the navigation. */
    private int mErrorCode;
    private int mLastHttpStatusCode;

    private WebappOfflineDialog mOfflineDialog;

    /** Indicates whether reloading is allowed. */
    private boolean mAllowReloads;

    private String mAppName;

    private @ActivityType int mActivityType;

    public WebappSplashScreenController(WebappActivity webappActivity) {
        mWebappUma = new WebappUma();
        mWebappActivity = webappActivity;
    }

    public boolean isInSplashScreenCapturePhase() {
        return mInSplashScreenCapturePhase;
    }

    /** Shows the splash screen. */
    public void showSplashScreen(@ActivityType int activityType, ViewGroup parentView) {
        WebappInfo webappInfo = mWebappActivity.mWebappInfo;
        mActivityType = activityType;
        mParentView = parentView;
        mAppName = webappInfo.name();

        Context context = ContextUtils.getApplicationContext();
        final int backgroundColor = ColorUtils.getOpaqueColor(webappInfo.backgroundColor(
                ApiCompatibilityUtils.getColor(context.getResources(), R.color.webapp_default_bg)));

        mSplashScreen = new FrameLayout(context);
        mSplashScreen.setBackgroundColor(backgroundColor);
        // Consume all touch, don't pass them to web contents.
        mSplashScreen.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                v.performClick();
                return true;
            }
        });
        mParentView.addView(mSplashScreen);

        mWebappUma.splashscreenVisible();
        mWebappUma.recordSplashscreenBackgroundColor(webappInfo.hasValidBackgroundColor()
                        ? WebappUma.SPLASHSCREEN_COLOR_STATUS_CUSTOM
                        : WebappUma.SPLASHSCREEN_COLOR_STATUS_DEFAULT);
        mWebappUma.recordSplashscreenThemeColor(webappInfo.hasValidThemeColor()
                        ? WebappUma.SPLASHSCREEN_COLOR_STATUS_CUSTOM
                        : WebappUma.SPLASHSCREEN_COLOR_STATUS_DEFAULT);

        if (useHtmlSplashScreen()) {
            initializeHtmlLayout();
            return;
        }

        WebappDataStorage storage =
                WebappRegistry.getInstance().getWebappDataStorage(webappInfo.id());
        if (storage == null) {
            initializeIconLayout(webappInfo, backgroundColor, null);
            return;
        }

        storage.getSplashScreenImage(new WebappDataStorage.FetchCallback<Bitmap>() {
            @Override
            public void onDataRetrieved(Bitmap splashImage) {
                initializeIconLayout(webappInfo, backgroundColor, splashImage);
            }
        });
    }

    /** Should be called once native has loaded. */
    public void onFinishedNativeInit(Tab tab, CompositorViewHolder compositorViewHolder) {
        mNativeLoaded = true;
        mCompositorViewHolder = compositorViewHolder;
        tab.addObserver(this);
        if (mInitializedLayout) {
            mWebappUma.commitMetrics();
        }
    }

    @VisibleForTesting
    ViewGroup getSplashScreenForTests() {
        return mSplashScreen;
    }

    @Override
    public void didFirstVisuallyNonEmptyPaint(Tab tab) {
        // didFirstVisuallyNonEmptyPaint is not good enough for timing of splash screen capture.
        if (!mInSplashScreenCapturePhase && canHideSplashScreen()) {
            hideSplashScreen(tab, WebappUma.SPLASHSCREEN_HIDES_REASON_PAINT);
        }
    }

    @Override
    public void onPageLoadFinished(Tab tab) {
        // onPageLoadFinished is not good enough for timing of splash screen capture.
        if (!mInSplashScreenCapturePhase && canHideSplashScreen()) {
            hideSplashScreen(tab, WebappUma.SPLASHSCREEN_HIDES_REASON_LOAD_FINISHED);
        }
    }

    @Override
    public void onDidSwapAfterLoad(Tab tab) {
        // Only onDidSwapAfterLoad ensures all pixels are flushed to the screen.
        if (mInSplashScreenCapturePhase && canHideSplashScreen()) {
            hideSplashScreen(tab, WebappUma.SPLASHSCREEN_HIDES_REASON_LOAD_FINISHED);
        }
    }

    @Override
    public void onPageLoadFailed(Tab tab, int errorCode) {
        if (canHideSplashScreen()) {
            hideSplashScreen(tab, WebappUma.SPLASHSCREEN_HIDES_REASON_LOAD_FAILED);
        }
    }

    @Override
    public void onCrash(Tab tab, boolean sadTabShown) {
        hideSplashScreen(tab, WebappUma.SPLASHSCREEN_HIDES_REASON_CRASH);
    }

    @Override
    public void onDidFinishNavigation(final Tab tab, final String url, boolean isInMainFrame,
            boolean isErrorPage, boolean hasCommitted, boolean isSameDocument,
            boolean isFragmentNavigation, Integer pageTransition, int errorCode,
            int httpStatusCode) {
        mLastHttpStatusCode = httpStatusCode;
        if (mActivityType == WebappActivity.ACTIVITY_TYPE_WEBAPP) return;

        mErrorCode = errorCode;

        switch (mErrorCode) {
            case NetError.ERR_NETWORK_CHANGED:
                onNetworkChanged(tab);
                break;
            case NetError.ERR_INTERNET_DISCONNECTED:
                onNetworkDisconnected(tab);
                break;
            default:
                if (mOfflineDialog != null) {
                    mOfflineDialog.cancel();
                    mOfflineDialog = null;
                }
                break;
        }
    }

    protected boolean canHideSplashScreen() {
        if (mActivityType == WebappActivity.ACTIVITY_TYPE_WEBAPP) return true;
        return mErrorCode != NetError.ERR_INTERNET_DISCONNECTED
                && mErrorCode != NetError.ERR_NETWORK_CHANGED;
    }

    private void onNetworkChanged(Tab tab) {
        if (!mAllowReloads) return;

        // It is possible that we get {@link NetError.ERR_NETWORK_CHANGED} during the first
        // reload after the device is online. The navigation will fail until the next auto
        // reload fired by {@link NetErrorHelperCore}. We call reload explicitly to reduce the
        // waiting time.
        tab.reloadIgnoringCache();
        mAllowReloads = false;
    }

    private void onNetworkDisconnected(final Tab tab) {
        if (mOfflineDialog != null || tab.getActivity() == null) return;

        final NetworkChangeNotifier.ConnectionTypeObserver observer =
                new NetworkChangeNotifier.ConnectionTypeObserver() {
                    @Override
                    public void onConnectionTypeChanged(int connectionType) {
                        if (!NetworkChangeNotifier.isOnline()) return;

                        NetworkChangeNotifier.removeConnectionTypeObserver(this);
                        tab.reloadIgnoringCache();
                        // One more reload is allowed after the network connection is back.
                        mAllowReloads = true;
                    }
                };

        NetworkChangeNotifier.addConnectionTypeObserver(observer);
        mOfflineDialog = new WebappOfflineDialog();
        mOfflineDialog.show(
                tab.getActivity(), mAppName, mActivityType == WebappActivity.ACTIVITY_TYPE_WEBAPK);
    }

    /** Sets the splash screen layout and sets the splash screen's title and icon. */
    private void initializeIconLayout(
            WebappInfo webappInfo, int backgroundColor, Bitmap splashImage) {
        mInitializedLayout = true;
        Context context = ContextUtils.getApplicationContext();
        Resources resources = context.getResources();

        Bitmap displayIcon = (splashImage == null) ? webappInfo.icon() : splashImage;
        int minimiumSizeThreshold =
                resources.getDimensionPixelSize(R.dimen.webapp_splash_image_size_minimum);
        int bigThreshold =
                resources.getDimensionPixelSize(R.dimen.webapp_splash_image_size_threshold);

        // Inflate the correct layout for the image.
        int layoutId;
        if (displayIcon == null || displayIcon.getWidth() < minimiumSizeThreshold
                || (displayIcon == webappInfo.icon() && webappInfo.isIconGenerated())) {
            mWebappUma.recordSplashscreenIconType(WebappUma.SPLASHSCREEN_ICON_TYPE_NONE);
            layoutId = R.layout.webapp_splash_screen_no_icon;
        } else {
            // The size of the splash screen image determines which layout to use.
            boolean isUsingSmallSplashImage = displayIcon.getWidth() <= bigThreshold
                    || displayIcon.getHeight() <= bigThreshold;
            if (isUsingSmallSplashImage) {
                layoutId = R.layout.webapp_splash_screen_small;
            } else {
                layoutId = R.layout.webapp_splash_screen_large;
            }

            // Record stats about the splash screen.
            int splashScreenIconType;
            if (splashImage == null) {
                splashScreenIconType = WebappUma.SPLASHSCREEN_ICON_TYPE_FALLBACK;
            } else if (isUsingSmallSplashImage) {
                splashScreenIconType = WebappUma.SPLASHSCREEN_ICON_TYPE_CUSTOM_SMALL;
            } else {
                splashScreenIconType = WebappUma.SPLASHSCREEN_ICON_TYPE_CUSTOM;
            }
            mWebappUma.recordSplashscreenIconType(splashScreenIconType);
            mWebappUma.recordSplashscreenIconSize(
                    Math.round(displayIcon.getWidth() / resources.getDisplayMetrics().density));
        }

        ViewGroup subLayout =
                (ViewGroup) LayoutInflater.from(context).inflate(layoutId, mSplashScreen, true);

        // Set up the elements of the splash screen.
        TextView appNameView = (TextView) subLayout.findViewById(R.id.webapp_splash_screen_name);
        ImageView splashIconView =
                (ImageView) subLayout.findViewById(R.id.webapp_splash_screen_icon);
        appNameView.setText(webappInfo.name());
        if (splashIconView != null) splashIconView.setImageBitmap(displayIcon);

        if (ColorUtils.shouldUseLightForegroundOnBackground(backgroundColor)) {
            appNameView.setTextColor(
                    ApiCompatibilityUtils.getColor(resources, R.color.webapp_splash_title_light));
        }

        if (mNativeLoaded) {
            mWebappUma.commitMetrics();
        }
    }

    private boolean useHtmlSplashScreen() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            // Don't use HTML splash screen in multi window or picture in picture scenarios
            // due to unpredictable size of the viewport.
            if (mWebappActivity.isInMultiWindowMode() || mWebappActivity.isInPictureInPictureMode())
                return false;
        }

        return /*ChromeFeatureList.isEnabled(ChromeFeatureList.PWA_IMPROVED_SPLASH_SCREEN)
                &&*/ mWebappActivity.mWebappInfo.hasSplashScreenUri();
    }

    private void initializeHtmlLayout() {
        mInitializedLayout = true;

        mWebappUma.recordSplashscreenIconType(WebappUma.SPLASHSCREEN_ICON_TYPE_NONE);

        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            File splashScreenFile = getSplashScreenFile(
                    mWebappActivity.getResources().getConfiguration().orientation);

            if (!splashScreenFile.exists()) {
                mInSplashScreenCapturePhase = true;
                return;
            }

            Bitmap bmp = BitmapFactory.decodeFile(splashScreenFile.getAbsolutePath());
            ImageView imageView = new ImageView(mWebappActivity);
            imageView.setImageBitmap(bmp);
            mSplashScreen.addView(imageView);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    private void saveSplashScreenImage(final Bitmap image) {
        new AsyncTask<Void, Void, File>() {
            @Override
            protected File doInBackground(Void... params) {
                FileOutputStream fOut = null;
                try {
                    File splashScreenFile = getSplashScreenFile(image.getWidth() > image.getHeight()
                                    ? Configuration.ORIENTATION_LANDSCAPE
                                    : Configuration.ORIENTATION_PORTRAIT);
                    File parent = splashScreenFile.getParentFile();
                    if (parent.exists() || parent.mkdir()) {
                        fOut = new FileOutputStream(splashScreenFile);
                        image.compress(Bitmap.CompressFormat.JPEG, 85, fOut);
                        return splashScreenFile;
                    }
                } catch (IOException e) {
                    Log.w(TAG, "IOException while saving screenshot", e);
                } finally {
                    StreamUtil.closeQuietly(fOut);
                }

                return null;
            }
        }
                .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private File getSplashScreenFile(int orientation) throws IOException {
        return new File(UiUtils.getDirectoryForImageCapture(mWebappActivity)
                + "/webapp_splash_screen/" + mWebappActivity.mWebappInfo.id() + "_ss_"
                + (orientation == Configuration.ORIENTATION_PORTRAIT ? "p" : "l") + ".jpg");
    }

    private void hideSplashScreen(final Tab tab, final int reason) {
        if (mSplashScreen == null) return;

        if (!mInSplashScreenCapturePhase) {
            // If we don't capture screenshot we show the webpage on first paint or load, which do
            // not ensure frame has been swapped out of compositor. surfaceRedrawNeededAsync ensures
            // flash of white does not happen (crbug.com/734500).
            mCompositorViewHolder.getCompositorView().surfaceRedrawNeededAsync(null, () -> {
                if (mSplashScreen == null) return;
                mParentView.removeView(mSplashScreen);
                tab.removeObserver(WebappSplashScreenController.this);
                mSplashScreen = null;
                mCompositorViewHolder = null;
                mWebappUma.splashscreenHidden(reason);
                // Clear the history in case we've been on the splash screen url for capture.
                mWebappActivity.getActivityTab()
                        .getWebContents()
                        .getNavigationController()
                        .clearHistory();
            });
            return;
        }

        if ((reason == WebappUma.SPLASHSCREEN_HIDES_REASON_LOAD_FAILED
                    || reason == WebappUma.SPLASHSCREEN_HIDES_REASON_CRASH
                    || (mLastHttpStatusCode >= 400))) {
            // We don't capture the splash screen if the page didn't load successfully
            // or it crashed.
            navigateToStartUrl();
            return;
        }

        mSplashScreen.setAlpha(0); // Using splash screen element as 'glass', blocks interactions.
        WebContents webContents = mWebappActivity.getActivityTab().getWebContents();
        webContents.getContentBitmapAsync(0, 0, new ContentBitmapCallback() {
            @Override
            public void onFinishGetBitmap(Bitmap bitmap, int response) {
                if (bitmap != null) {
                    saveSplashScreenImage(bitmap);
                } else {
                    Log.w(TAG, "Failed to capture content bitmap: " + response);
                }

                navigateToStartUrl();
            }
        });
    }

    private void navigateToStartUrl() {
        mInSplashScreenCapturePhase = false;
        mWebappActivity.getActivityTab().loadUrl(new LoadUrlParams(
                mWebappActivity.mWebappInfo.uri().toString(), PageTransition.AUTO_TOPLEVEL));
    }
}
