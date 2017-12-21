// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.annotation.SuppressLint;
import android.graphics.Point;
import android.os.StrictMode;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.FrameLayout;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.compositor.CompositorView;
import org.chromium.chrome.browser.page_info.PageInfoPopup;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.InterceptNavigationDelegateImpl;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tab.TabRedirectHandler;
import org.chromium.chrome.browser.tabmodel.ChromeTabCreator;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager.TabCreator;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.common.BrowserControlsState;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.WindowAndroid.PermissionCallback;
import org.chromium.ui.display.DisplayAndroid;
import org.chromium.ui.display.VirtualDisplayAndroid;

/**
 * This view extends from GvrLayout which wraps a GLSurfaceView that renders VR shell.
 */
@JNINamespace("vr_shell")
public class VrShellImpl extends GvrLayout implements VrShell, SurfaceHolder.Callback {
    private static final String TAG = "VrShellImpl";
    private static final float INCHES_TO_METERS = 0.0254f;

    private final ChromeActivity mActivity;
    private final CompositorView mCompositorView;
    private final VrShellDelegate mDelegate;
    private final VirtualDisplayAndroid mContentVirtualDisplay;
    private final TabRedirectHandler mTabRedirectHandler;
    private final TabObserver mTabObserver;
    private final TabModelSelectorObserver mTabModelSelectorObserver;
    private final View.OnTouchListener mTouchListener;
    private TabModelSelectorTabObserver mTabModelSelectorTabObserver;

    private long mNativeVrShell;

    private View mPresentationView;

    // The tab that holds the main ContentViewCore.
    private Tab mTab;
    private ContentViewCore mContentViewCore;
    private Boolean mCanGoBack;
    private Boolean mCanGoForward;

    private VrWindowAndroid mContentVrWindowAndroid;

    private boolean mReprojectedRendering;

    private InterceptNavigationDelegateImpl mNonVrInterceptNavigationDelegate;
    private TabRedirectHandler mNonVrTabRedirectHandler;
    private TabModelSelector mTabModelSelector;
    private float mLastContentWidth;
    private float mLastContentHeight;
    private float mLastContentDpr;
    private Boolean mPaused;
    private boolean mPendingVSyncPause;

    private AndroidUiGestureTarget mAndroidUiGestureTarget;

    private OnDispatchTouchEventCallback mOnDispatchTouchEventForTesting;

    private Surface mContentSurface;
    private VrViewContainer mNonVrViews;

    public VrShellImpl(
            ChromeActivity activity, VrShellDelegate delegate, TabModelSelector tabModelSelector) {
        super(activity);
        mActivity = activity;
        mDelegate = delegate;
        mTabModelSelector = tabModelSelector;

        mCompositorView = mActivity.getCompositorViewHolder().detachForVr();
        mCompositorView.detachForVr();
        mActivity.getToolbarManager().setProgressBarEnabled(false);

        mNonVrViews = (VrViewContainer) mActivity.findViewById(R.id.vr_view_container);
        mNonVrViews.onEnterVr();

        // This overrides the default intent created by GVR to return to Chrome when the DON flow
        // is triggered by resuming the GvrLayout, which is the usual way Daydream apps enter VR.
        // See VrShellDelegate#getEnterVrPendingIntent for why we need to do this.
        setReentryIntent(VrShellDelegate.getEnterVrPendingIntent(activity));

        mReprojectedRendering = setAsyncReprojectionEnabled(true);
        if (mReprojectedRendering) {
            // No need render to a Surface if we're reprojected. We'll be rendering with surfaceless
            // EGL.
            mPresentationView = new FrameLayout(mActivity);

            // Only enable sustained performance mode when Async reprojection decouples the app
            // framerate from the display framerate.
            AndroidCompat.setSustainedPerformanceMode(mActivity, true);
        } else {
            SurfaceView surfaceView = new SurfaceView(mActivity);
            surfaceView.getHolder().addCallback(this);
            mPresentationView = surfaceView;
        }

        setPresentationView(mPresentationView);

        getUiLayout().setCloseButtonListener(mDelegate.getVrCloseButtonListener());

        DisplayAndroid primaryDisplay = DisplayAndroid.getNonMultiDisplay(activity);
        mContentVirtualDisplay = VirtualDisplayAndroid.createVirtualDisplay();
        mContentVirtualDisplay.setTo(primaryDisplay);

        // Set the initial content size and DPR to be applied to reparented tabs. Otherwise, Chrome
        // will crash due to a GL buffer initialized with zero bytes.
        ContentViewCore activeContentViewCore =
                mActivity.getActivityTab().getActiveContentViewCore();
        assert activeContentViewCore != null;
        WindowAndroid wa = activeContentViewCore.getWindowAndroid();
        mLastContentDpr = wa != null ? wa.getDisplay().getDipScale() : 1.0f;
        mLastContentWidth = activeContentViewCore.getViewportWidthPix() / mLastContentDpr;
        mLastContentHeight = activeContentViewCore.getViewportHeightPix() / mLastContentDpr;

        mTabRedirectHandler = new TabRedirectHandler(mActivity) {
            @Override
            public boolean shouldStayInChrome(boolean hasExternalProtocol) {
                return true;
            }
        };

        mTabObserver = new EmptyTabObserver() {
            @Override
            public void onContentChanged(Tab tab) {
                // Restore proper focus on the old CVC.
                if (mContentViewCore != null) mContentViewCore.onWindowFocusChanged(false);
                mContentViewCore = null;
                if (mNativeVrShell == 0) return;
                if (tab.isShowingSadTab()) {
                    // For now we don't support the sad tab page. crbug.com/661609.
                    forceExitVr();
                    return;
                }
                setContentCssSize(mLastContentWidth, mLastContentHeight, mLastContentDpr);
                if (tab.getContentViewCore() != null) {
                    mContentViewCore = tab.getContentViewCore();
                    mContentViewCore.getContainerView().requestFocus();
                    // We need the CVC to think it has Window Focus so it doesn't blur the page,
                    // even though we're drawing VR layouts over top of it.
                    mContentViewCore.onWindowFocusChanged(true);
                }
                nativeSwapContents(mNativeVrShell, tab,
                        DisplayAndroid.getNonMultiDisplay(activity).getDipScale());
                updateHistoryButtonsVisibility();
            }

            @Override
            public void onWebContentsSwapped(Tab tab, boolean didStartLoad, boolean didFinishLoad) {
                onContentChanged(tab);
            }

            @Override
            public void onLoadProgressChanged(Tab tab, int progress) {
                if (mNativeVrShell == 0) return;
                nativeOnLoadProgressChanged(mNativeVrShell, progress / 100.0);
            }

            @Override
            public void onCrash(Tab tab, boolean sadTabShown) {
                updateHistoryButtonsVisibility();
            }

            @Override
            public void onLoadStarted(Tab tab, boolean toDifferentDocument) {
                if (!toDifferentDocument) return;
                updateHistoryButtonsVisibility();
            }

            @Override
            public void onLoadStopped(Tab tab, boolean toDifferentDocument) {
                if (!toDifferentDocument) return;
                updateHistoryButtonsVisibility();
            }

            @Override
            public void onUrlUpdated(Tab tab) {
                updateHistoryButtonsVisibility();
            }
        };

        mTabModelSelectorObserver = new EmptyTabModelSelectorObserver() {
            @Override
            public void onChange() {
                swapToForegroundTab();
            }

            @Override
            public void onNewTabCreated(Tab tab) {
                if (mNativeVrShell == 0) return;
                nativeOnTabUpdated(mNativeVrShell, tab.isIncognito(), tab.getId(), tab.getTitle());
            }
        };

        mTouchListener = new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                    nativeOnTriggerEvent(mNativeVrShell, true);
                    return true;
                } else if (event.getActionMasked() == MotionEvent.ACTION_UP
                        || event.getActionMasked() == MotionEvent.ACTION_CANCEL) {
                    nativeOnTriggerEvent(mNativeVrShell, false);
                    return true;
                }
                return false;
            }
        };
    }

    @Override
    // TODO(crbug.com/762588): Fix getRealMetrics and remove suppression.
    @SuppressLint("NewApi")
    public void initializeNative(Tab currentTab, boolean forWebVr,
            boolean webVrAutopresentationExpected, boolean inCct) {
        assert currentTab != null;
        // Get physical and pixel size of the display, which is needed by native
        // to dynamically calculate the content's resolution and window size.
        DisplayMetrics dm = new DisplayMetrics();
        mActivity.getWindowManager().getDefaultDisplay().getRealMetrics(dm);
        float displayWidthMeters = (dm.widthPixels / dm.xdpi) * INCHES_TO_METERS;
        float displayHeightMeters = (dm.heightPixels / dm.ydpi) * INCHES_TO_METERS;

        mContentVrWindowAndroid = new VrWindowAndroid(mActivity, mContentVirtualDisplay);
        mCompositorView.replaceCompositorWindowForVr(mContentVrWindowAndroid.getNativePointer());
        boolean browsingDisabled = !VrShellDelegate.isVrShellEnabled(mDelegate.getVrSupportLevel());
        boolean hasOrCanRequestAudioPermission =
                mActivity.getWindowAndroid().hasPermission(android.Manifest.permission.RECORD_AUDIO)
                || mActivity.getWindowAndroid().canRequestPermission(
                           android.Manifest.permission.RECORD_AUDIO);
        mNativeVrShell = nativeInit(mDelegate, forWebVr, webVrAutopresentationExpected, inCct,
                browsingDisabled, hasOrCanRequestAudioPermission, getGvrApi().getNativeGvrContext(),
                mReprojectedRendering, displayWidthMeters, displayHeightMeters, dm.widthPixels,
                dm.heightPixels);

        reparentAllTabs(mContentVrWindowAndroid);
        swapToTab(currentTab);
        createTabList();
        mActivity.getTabModelSelector().addObserver(mTabModelSelectorObserver);
        createTabModelSelectorTabObserver();
        updateHistoryButtonsVisibility();

        mPresentationView.setOnTouchListener(mTouchListener);

        mAndroidUiGestureTarget = new AndroidUiGestureTarget(mNonVrViews.getInputTarget(),
                mContentVrWindowAndroid.getDisplay().getDipScale(), getNativePageScrollRatio());
        nativeSetAndroidGestureTarget(mNativeVrShell, mAndroidUiGestureTarget);
    }

    private void createTabList() {
        assert mNativeVrShell != 0;
        TabModel main = mTabModelSelector.getModel(false);
        int count = main.getCount();
        Tab[] mainTabs = new Tab[count];
        for (int i = 0; i < count; ++i) {
            mainTabs[i] = main.getTabAt(i);
        }
        TabModel incognito = mTabModelSelector.getModel(true);
        count = incognito.getCount();
        Tab[] incognitoTabs = new Tab[count];
        for (int i = 0; i < count; ++i) {
            incognitoTabs[i] = incognito.getTabAt(i);
        }
        nativeOnTabListCreated(mNativeVrShell, mainTabs, incognitoTabs);
    }

    private void swapToForegroundTab() {
        Tab tab = mActivity.getActivityTab();
        if (tab == mTab) return;
        if (tab == null || !mDelegate.canEnterVr(tab, false)) {
            forceExitVr();
            return;
        }
        swapToTab(tab);
    }

    private void swapToTab(Tab tab) {
        assert tab != null;
        if (mTab != null) {
            mTab.removeObserver(mTabObserver);
            restoreTabFromVR();
            mTab.updateFullscreenEnabledState();
        }

        mTab = tab;
        initializeTabForVR();
        mTab.addObserver(mTabObserver);
        mTab.updateFullscreenEnabledState();
        mTabObserver.onContentChanged(mTab);
    }

    private void initializeTabForVR() {
        mNonVrInterceptNavigationDelegate = mTab.getInterceptNavigationDelegate();
        mTab.setInterceptNavigationDelegate(
                new InterceptNavigationDelegateImpl(new VrExternalNavigationDelegate(mTab), mTab));
        // Make sure we are not redirecting to another app, i.e. out of VR mode.
        mNonVrTabRedirectHandler = mTab.getTabRedirectHandler();
        mTab.setTabRedirectHandler(mTabRedirectHandler);
        assert mTab.getWindowAndroid() == mContentVrWindowAndroid;
    }

    private void restoreTabFromVR() {
        mTab.setInterceptNavigationDelegate(mNonVrInterceptNavigationDelegate);
        mTab.setTabRedirectHandler(mNonVrTabRedirectHandler);
        mNonVrTabRedirectHandler = null;
    }

    private void reparentAllTabs(WindowAndroid window) {
        // Ensure new tabs are created with the correct window.
        boolean[] values = {true, false};
        for (boolean incognito : values) {
            TabCreator tabCreator = mActivity.getTabCreator(incognito);
            if (tabCreator instanceof ChromeTabCreator) {
                ((ChromeTabCreator) tabCreator).setWindowAndroid(window);
            }
        }

        // Reparent all existing tabs.
        for (TabModel model : mActivity.getTabModelSelector().getModels()) {
            for (int i = 0; i < model.getCount(); ++i) {
                model.getTabAt(i).updateWindowAndroid(window);
            }
        }
    }

    // Returns true if Chrome has permission to use audio input.
    @CalledByNative
    public boolean hasAudioPermission() {
        return mDelegate.hasAudioPermission();
    }

    // Exits VR, telling the user to remove their headset, and returning to Chromium.
    @CalledByNative
    public void forceExitVr() {
        VrShellDelegate.showDoffAndExitVr(false);
    }

    // Called because showing PageInfo isn't supported in VR. This happens when the user clicks on
    // the security icon in the URL bar.
    @CalledByNative
    public void onUnhandledPageInfo() {
        VrShellDelegate.requestToExitVr(new OnExitVrRequestListener() {
            @Override
            public void onSucceeded() {
                PageInfoPopup.show(
                        mActivity, mActivity.getActivityTab(), null, PageInfoPopup.OPENED_FROM_VR);
            }

            @Override
            public void onDenied() {}
        }, UiUnsupportedMode.UNHANDLED_PAGE_INFO);
    }

    // Called because showing audio permission dialog isn't supported in VR. This happens when
    // the user wants to do a voice search.
    @CalledByNative
    public void onUnhandledPermissionPrompt() {
        VrShellDelegate.requestToExitVr(new OnExitVrRequestListener() {
            @Override
            public void onSucceeded() {
                PermissionCallback callback = new PermissionCallback() {
                    @Override
                    public void onRequestPermissionsResult(
                            String[] permissions, int[] grantResults) {
                        ThreadUtils.postOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                VrShellDelegate.enterVrIfNecessary();
                            }
                        });
                    }
                };
                String[] permissionArray = new String[1];
                permissionArray[0] = android.Manifest.permission.RECORD_AUDIO;
                mActivity.getWindowAndroid().requestPermissions(permissionArray, callback);
            }

            @Override
            public void onDenied() {}
        }, UiUnsupportedMode.VOICE_SEARCH_NEEDS_RECORD_AUDIO_OS_PERMISSION);
    }

    // Exits CCT, returning to the app that opened it.
    @CalledByNative
    public void exitCct() {
        mDelegate.exitCct();
    }

    @CalledByNative
    public void setContentCssSize(float width, float height, float dpr) {
        ThreadUtils.assertOnUiThread();
        mLastContentWidth = width;
        mLastContentHeight = height;
        mLastContentDpr = dpr;

        // Java views don't listen to our DPR changes, so to get them to render at the correct
        // size we need to make them larger.
        DisplayAndroid primaryDisplay = DisplayAndroid.getNonMultiDisplay(mActivity);
        float dip = primaryDisplay.getDipScale();

        int contentWidth = (int) Math.ceil(width * dpr);
        int contentHeight = (int) Math.ceil(height * dpr);

        int overlayWidth = Math.min(2560, (int) Math.ceil(width * dip));
        int overlayHeight = Math.min(1440, (int) Math.ceil(height * dip));

        mNonVrViews.resizeForVr(overlayWidth, overlayHeight);

        Point size = new Point(contentWidth, contentHeight);
        mContentVirtualDisplay.update(size, dpr, null, null, null, null, null, null);
        nativeOnPhysicalBackingSizeChanged(
                mNativeVrShell, mTab.getWebContents(), contentWidth, contentHeight);
        nativeBufferBoundsChanged(
                mNativeVrShell, contentWidth, contentHeight, overlayWidth, overlayHeight);
        if (mContentSurface != null) {
            mCompositorView.surfaceChanged(mContentSurface, -1, contentWidth, contentHeight);
        }
    }

    @CalledByNative
    public void contentSurfaceCreated(Surface surface) {
        mContentSurface = surface;
        mCompositorView.surfaceCreated();
        int width = (int) Math.ceil(mLastContentWidth * mLastContentDpr);
        int height = (int) Math.ceil(mLastContentHeight * mLastContentDpr);
        mCompositorView.surfaceChanged(surface, -1, width, height);
    }

    @CalledByNative
    public void contentOverlaySurfaceCreated(Surface surface) {
        mNonVrViews.setSurface(surface);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        boolean parentConsumed = super.dispatchTouchEvent(event);
        if (mOnDispatchTouchEventForTesting != null) {
            mOnDispatchTouchEventForTesting.onDispatchTouchEvent(parentConsumed);
        }
        return parentConsumed;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (mTab.getContentViewCore() != null
                && mTab.getContentViewCore().dispatchKeyEvent(event)) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if (mTab.getContentViewCore() != null
                && mTab.getContentViewCore().onGenericMotionEvent(event)) {
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    public void onResume() {
        if (mPaused != null && !mPaused) return;
        mPaused = false;
        super.onResume();
        if (mNativeVrShell != 0) {
            // Refreshing the viewer profile may accesses disk under some circumstances outside of
            // our control.
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            try {
                nativeOnResume(mNativeVrShell);
            } finally {
                StrictMode.setThreadPolicy(oldPolicy);
            }
        }
    }

    @Override
    public void onPause() {
        if (mPaused != null && mPaused) return;
        mPaused = true;
        super.onPause();
        if (mNativeVrShell != 0) {
            nativeOnPause(mNativeVrShell);
        }
    }

    @Override
    public void shutdown() {
        mNonVrViews.onExitVr();
        mNonVrViews.setSurface(null);
        mActivity.getFullscreenManager().setPersistentFullscreenMode(false);
        reparentAllTabs(mActivity.getWindowAndroid());
        if (mNativeVrShell != 0) {
            nativeDestroy(mNativeVrShell);
            mNativeVrShell = 0;
        }
        mTabModelSelector.removeObserver(mTabModelSelectorObserver);
        mTabModelSelectorTabObserver.destroy();
        assert mTab != null;
        mTab.removeObserver(mTabObserver);
        restoreTabFromVR();
        if (mTab.getContentViewCore() != null) {
            View parent = mTab.getContentViewCore().getContainerView();
            mTab.getWebContents().setSize(parent.getWidth(), parent.getHeight());
        }
        mTab.updateBrowserControlsState(BrowserControlsState.SHOWN, true);

        mContentVirtualDisplay.destroy();

        mCompositorView.onExitVr();
        if (mActivity.getCompositorViewHolder() != null) {
            mActivity.getCompositorViewHolder().onExitVr();
        }
        mActivity.getToolbarManager().setProgressBarEnabled(true);
        super.shutdown();
    }

    @Override
    public void pause() {
        onPause();
    }

    @Override
    public void resume() {
        onResume();
    }

    @Override
    public void teardown() {
        shutdown();
    }

    @Override
    public void setWebVrModeEnabled(boolean enabled, boolean showToast) {
        mContentVrWindowAndroid.setVSyncPaused(enabled);

        nativeSetWebVrMode(mNativeVrShell, enabled, showToast);
    }

    @Override
    public boolean getWebVrModeEnabled() {
        return nativeGetWebVrMode(mNativeVrShell);
    }

    @Override
    public boolean isDisplayingUrlForTesting() {
        return nativeIsDisplayingUrlForTesting(mNativeVrShell);
    }

    @Override
    public FrameLayout getContainer() {
        return this;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        nativeSetSurface(mNativeVrShell, holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // No need to do anything here, we don't care about surface width/height.
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mCompositorView.surfaceDestroyed();
        nativeSetSurface(mNativeVrShell, null);
    }

    private void createTabModelSelectorTabObserver() {
        assert mTabModelSelectorTabObserver == null;
        mTabModelSelectorTabObserver = new TabModelSelectorTabObserver(mTabModelSelector) {
            @Override
            public void onTitleUpdated(Tab tab) {
                if (mNativeVrShell == 0) return;
                nativeOnTabUpdated(mNativeVrShell, tab.isIncognito(), tab.getId(), tab.getTitle());
            }

            @Override
            public void onClosingStateChanged(Tab tab, boolean closing) {
                if (mNativeVrShell == 0) return;
                if (closing) {
                    nativeOnTabRemoved(mNativeVrShell, tab.isIncognito(), tab.getId());
                } else {
                    nativeOnTabUpdated(mNativeVrShell, tab.isIncognito(), tab.getId(),
                            tab.getTitle());
                }
            }

            @Override
            public void onDestroyed(Tab tab) {
                if (mNativeVrShell == 0) return;
                nativeOnTabRemoved(mNativeVrShell, tab.isIncognito(), tab.getId());
            }
        };
    }

    @CalledByNative
    public boolean hasDaydreamSupport() {
        return mDelegate.hasDaydreamSupport();
    }

    @Override
    public void requestToExitVr(@UiUnsupportedMode int reason) {
        nativeRequestToExitVr(mNativeVrShell, reason);
    }

    @CalledByNative
    private void onExitVrRequestResult(@UiUnsupportedMode int reason, boolean shouldExit) {
        if (shouldExit) {
            nativeLogUnsupportedModeUserMetric(mNativeVrShell, reason);
        }
        mDelegate.onExitVrRequestResult(shouldExit);
    }

    @CalledByNative
    private void showTab(int id) {
        Tab tab = mActivity.getTabModelSelector().getTabById(id);
        if (tab == null) {
            return;
        }
        int index = mActivity.getTabModelSelector().getModel(tab.isIncognito()).indexOf(tab);
        if (index == TabModel.INVALID_TAB_INDEX) {
            return;
        }
        TabModelUtils.setIndex(mActivity.getTabModelSelector().getModel(tab.isIncognito()), index);
    }

    @CalledByNative
    private void openNewTab(boolean incognito) {
        mActivity.getTabCreator(incognito).launchUrl(
                UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
    }

    @CalledByNative
    private void loadUrl(String url) {
        assert mTab != null;
        mTab.loadUrl(new LoadUrlParams(url));
    }

    @VisibleForTesting
    @Override
    @CalledByNative
    public void navigateForward() {
        if (!mCanGoForward) return;
        mActivity.getToolbarManager().forward();
        updateHistoryButtonsVisibility();
    }

    @VisibleForTesting
    @Override
    @CalledByNative
    public void navigateBack() {
        if (!mCanGoBack) return;
        if (mActivity instanceof ChromeTabbedActivity) {
            // TODO(mthiesse): We should do this for custom tabs as well, as back for custom tabs
            // is also expected to close tabs.
            ((ChromeTabbedActivity) mActivity).handleBackPressed();
        } else {
            mActivity.getToolbarManager().back();
        }
        updateHistoryButtonsVisibility();
    }

    private void updateHistoryButtonsVisibility() {
        assert mTab != null;
        boolean willCloseTab = false;
        if (mActivity instanceof ChromeTabbedActivity) {
            // If hitting back would minimize Chrome, disable the back button.
            // See ChromeTabbedActivity#handleBackPressed().
            willCloseTab = ChromeTabbedActivity.backShouldCloseTab(mTab)
                    && !mTab.isCreatedForExternalApp();
        }
        boolean canGoBack = mTab.canGoBack() || willCloseTab;
        boolean canGoForward = mTab.canGoForward();
        if ((mCanGoBack != null && canGoBack == mCanGoBack)
                && (mCanGoForward != null && canGoForward == mCanGoForward)) {
            return;
        }
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        nativeSetHistoryButtonsEnabled(mNativeVrShell, mCanGoBack, mCanGoForward);
    }

    @CalledByNative
    public void reload() {
        mTab.reload();
    }

    private float getNativePageScrollRatio() {
        return mActivity.getWindowAndroid().getDisplay().getDipScale()
                / mContentVrWindowAndroid.getDisplay().getDipScale();
    }

    /**
     * Sets the callback that will be run when VrShellImpl's dispatchTouchEvent
     * is run and the parent consumed the event.
     * @param callback The Callback to be run.
     */
    @VisibleForTesting
    public void setOnDispatchTouchEventForTesting(OnDispatchTouchEventCallback callback) {
        mOnDispatchTouchEventForTesting = callback;
    }

    @VisibleForTesting
    @Override
    public Boolean isBackButtonEnabled() {
        return mCanGoBack;
    }

    @VisibleForTesting
    @Override
    public Boolean isForwardButtonEnabled() {
        return mCanGoForward;
    }

    @VisibleForTesting
    public float getContentWidthForTesting() {
        return mLastContentWidth;
    }

    @VisibleForTesting
    public float getContentHeightForTesting() {
        return mLastContentHeight;
    }

    @VisibleForTesting
    public View getPresentationViewForTesting() {
        return mPresentationView;
    }

    private native long nativeInit(VrShellDelegate delegate, boolean forWebVR,
            boolean webVrAutopresentationExpected, boolean inCct, boolean browsingDisabled,
            boolean hasOrCanRequestAudioPermission, long gvrApi, boolean reprojectedRendering,
            float displayWidthMeters, float displayHeightMeters, int displayWidthPixels,
            int displayHeightPixels);
    private native void nativeSetSurface(long nativeVrShell, Surface surface);
    private native void nativeSwapContents(long nativeVrShell, Tab tab, float androidViewDipScale);
    private native void nativeSetAndroidGestureTarget(
            long nativeVrShell, AndroidUiGestureTarget androidUiGestureTarget);
    private native void nativeDestroy(long nativeVrShell);
    private native void nativeOnTriggerEvent(long nativeVrShell, boolean touched);
    private native void nativeOnPause(long nativeVrShell);
    private native void nativeOnResume(long nativeVrShell);
    private native void nativeOnLoadProgressChanged(long nativeVrShell, double progress);
    private native void nativeOnPhysicalBackingSizeChanged(
            long nativeVrShell, WebContents webContents, int width, int height);
    private native void nativeBufferBoundsChanged(long nativeVrShell, int contentWidth,
            int contentHeight, int overlayWidth, int overlayHeight);
    private native void nativeSetWebVrMode(long nativeVrShell, boolean enabled, boolean showToast);
    private native boolean nativeGetWebVrMode(long nativeVrShell);
    private native boolean nativeIsDisplayingUrlForTesting(long nativeVrShell);
    private native void nativeOnTabListCreated(long nativeVrShell, Tab[] mainTabs,
            Tab[] incognitoTabs);
    private native void nativeOnTabUpdated(long nativeVrShell, boolean incognito, int id,
            String title);
    private native void nativeOnTabRemoved(long nativeVrShell, boolean incognito, int id);
    private native void nativeSetHistoryButtonsEnabled(
            long nativeVrShell, boolean canGoBack, boolean canGoForward);
    private native void nativeRequestToExitVr(long nativeVrShell, @UiUnsupportedMode int reason);
    private native void nativeLogUnsupportedModeUserMetric(
            long nativeVrShell, @UiUnsupportedMode int mode);
}
