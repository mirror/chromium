// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.hosted;

import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;

import com.google.android.apps.chrome.R;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.browser.CompositorChromeActivity;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.Tab;
import org.chromium.chrome.browser.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.appmenu.AppMenuObserver;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerDocument;
import org.chromium.chrome.browser.document.BrandColorUtils;
import org.chromium.chrome.browser.tabmodel.SingleTabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarControlContainer;
import org.chromium.chrome.browser.toolbar.ToolbarHelper;
import org.chromium.chrome.browser.widget.findinpage.FindToolbarManager;
import org.chromium.content_public.browser.LoadUrlParams;

/**
 * The activity for hosted mode. It will be launched on top of a client's task.
 */
public class HostedActivity extends CompositorChromeActivity {
    private static HostedContentHandler sActiveContentHandler;

    private HostedTab mTab;
    private ToolbarHelper mToolbarHelper;
    private HostedAppMenuPropertiesDelegate mAppMenuPropertiesDelegate;
    private AppMenuHandler mAppMenuHandler;
    private FindToolbarManager mFindToolbarManager;
    private HostedIntentDataProvider mIntentDataProvider;
    private long mSessionId;
    private HostedContentHandler mHostedContentHandler;

    // This is to give the right package name while using the client's resources during an
    // overridePendingTransition call.
    // TODO(ianwen, yusufo): Figure out a solution to extract external resources without having to
    // change the package name.
    private boolean mShouldOverridePackage;

    /**
     * Sets the currently active {@link HostedContentHandler} in focus.
     * @param contentHandler {@link HostedContentHandler} to set.
     */
    public static void setActiveContentHandler(HostedContentHandler contentHandler) {
        sActiveContentHandler = contentHandler;
    }

    /**
     * Used to check whether an incoming intent can be handled by the
     * current {@link HostedContentHandler}.
     * @return Whether the active {@link HostedContentHandler} has handled the intent.
     */
    public static boolean handleInActiveContentIfNeeded(Intent intent) {
        if (sActiveContentHandler == null) return false;

        long intentSessionId = intent.getLongExtra(HostedIntentDataProvider.EXTRA_HOSTED_SESSION_ID,
                        HostedIntentDataProvider.INVALID_SESSION_ID);
        if (intentSessionId == HostedIntentDataProvider.INVALID_SESSION_ID) return false;

        if (sActiveContentHandler.getSessionId() != intentSessionId) return false;
        String url = IntentHandler.getUrlFromIntent(intent);
        if (TextUtils.isEmpty(url)) return false;
        sActiveContentHandler.loadUrl(new LoadUrlParams(url));
        return true;
    }

    @Override
    public void onStart() {
        super.onStart();
        ChromeBrowserConnection.getInstance(getApplication())
                .keepAliveForSessionId(mIntentDataProvider.getSessionId(),
                        mIntentDataProvider.getKeepAliveServiceIntent());
    }

    @Override
    public void onStop() {
        super.onStop();
        ChromeBrowserConnection.getInstance(getApplication())
                .dontKeepAliveForSessionId(mIntentDataProvider.getSessionId());
    }

    @Override
    public void preInflationStartup() {
        super.preInflationStartup();
        setTabModelSelector(new SingleTabModelSelector(this, false, true));
        mIntentDataProvider = new HostedIntentDataProvider(getIntent(), this);
    }

    @Override
    public void postInflationStartup() {
        super.postInflationStartup();
        ToolbarControlContainer controlContainer =
                ((ToolbarControlContainer) findViewById(R.id.control_container));
        mAppMenuPropertiesDelegate = new HostedAppMenuPropertiesDelegate(this,
                mIntentDataProvider.getMenuTitles());
        mAppMenuHandler = new AppMenuHandler(this, mAppMenuPropertiesDelegate, R.menu.hosted_menu);
        mToolbarHelper = new ToolbarHelper(this, controlContainer,
                mAppMenuHandler, mAppMenuPropertiesDelegate,
                getCompositorViewHolder().getInvalidator());
        mToolbarHelper.setThemeColor(mIntentDataProvider.getToolbarColor());
        setStatusBarColor(mIntentDataProvider.getToolbarColor());
        if (mIntentDataProvider.shouldShowActionButton()) {
            mToolbarHelper.addCustomActionButton(mIntentDataProvider.getActionButtonIcon(),
                    new OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            mIntentDataProvider.sendButtonPendingIntentWithUrl(
                                    getApplicationContext(), mTab.getUrl());
                        }
                    });
        }
        mAppMenuHandler.addObserver(new AppMenuObserver() {
            @Override
            public void onMenuVisibilityChanged(boolean isVisible) {
                if (!isVisible) {
                    mAppMenuPropertiesDelegate.onMenuDismissed();
                }
            }
        });
    }

    @Override
    public void finishNativeInitialization() {
        String url = IntentHandler.getUrlFromIntent(getIntent());
        mSessionId = mIntentDataProvider.getSessionId();
        mTab = new HostedTab(this, getWindowAndroid(), mSessionId, url, Tab.INVALID_TAB_ID);
        getTabModelSelector().setTab(mTab);

        ToolbarControlContainer controlContainer = (ToolbarControlContainer) findViewById(
                R.id.control_container);
        LayoutManagerDocument layoutDriver = new LayoutManagerDocument(getCompositorViewHolder());
        initializeCompositorContent(layoutDriver, findViewById(R.id.url_bar),
                (ViewGroup) findViewById(android.R.id.content), controlContainer);
        mFindToolbarManager = new FindToolbarManager(this, getTabModelSelector(),
                mToolbarHelper.getContextualMenuBar().getCustomSelectionActionModeCallback());
        controlContainer.setFindToolbarManager(mFindToolbarManager);
        mToolbarHelper.initializeControls(
                mFindToolbarManager, null, layoutDriver, null, null, null,
                new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        HostedActivity.this.finish();
                    }
                });

        mTab.setFullscreenManager(getFullscreenManager());
        mTab.loadUrl(new LoadUrlParams(url));
        mHostedContentHandler = new HostedContentHandler() {
            @Override
            public void loadUrl(LoadUrlParams params) {
                mTab.loadUrl(params);
            }

            @Override
            public long getSessionId() {
                return mSessionId;
            }
        };
        super.finishNativeInitialization();
    }

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();
        setActiveContentHandler(mHostedContentHandler);
    }

    @Override
    public void onStopWithNative() {
        super.onStopWithNative();
        setActiveContentHandler(null);
    }

    @Override
    protected void onDeferredStartup() {
        super.onDeferredStartup();
        mToolbarHelper.onDeferredStartup();
    }

    @Override
    public boolean hasDoneFirstDraw() {
        return mToolbarHelper.hasDoneFirstDraw();
    }

    /**
     * Calculate the proper color for status bar and update it. Only works on L and future versions.
     */
    private void setStatusBarColor(int color) {
        // If the client did not specify the toolbar color, we do not change the status bar color.
        if (color == getResources().getColor(R.color.default_primary_color)) return;

        ApiCompatibilityUtils.setStatusBarColor(getWindow(),
                BrandColorUtils.computeStatusBarColor(color));
    }

    @Override
    public SingleTabModelSelector getTabModelSelector() {
        return (SingleTabModelSelector) super.getTabModelSelector();
    }

    @Override
    protected int getControlContainerLayoutId() {
        return R.layout.hosted_control_container;
    }

    @Override
    protected int getControlContainerHeightResource() {
        return R.dimen.hosted_control_container_height;
    }

    @Override
    public String getPackageName() {
        if (mShouldOverridePackage) return mIntentDataProvider.getClientPackageName();
        return super.getPackageName();
    }

    @Override
    public void finish() {
        super.finish();
        if (mIntentDataProvider.shouldAnimateOnFinish()) {
            mShouldOverridePackage = true;
            overridePendingTransition(mIntentDataProvider.getAnimationEnterRes(),
                    mIntentDataProvider.getAnimationExitRes());
            mShouldOverridePackage = false;
        }
    }

    @Override
    protected boolean handleBackPressed() {
        if (mTab == null) return false;
        if (mTab.canGoBack()) {
            mTab.goBack();
        } else {
            finish();
        }
        return true;
    }

    @Override
    public boolean shouldShowAppMenu() {
        return mTab != null && mToolbarHelper.isInitialized();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int menuIndex = mAppMenuPropertiesDelegate.getIndexOfMenuItem(item);
        if (menuIndex >= 0) {
            mIntentDataProvider.clickMenuItemWithUrl(getApplicationContext(), menuIndex,
                    getTabModelSelector().getCurrentTab().getUrl());
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        if (id == R.id.show_menu) {
            if (shouldShowAppMenu()) {
                mAppMenuHandler.showAppMenu(mToolbarHelper.getMenuAnchor(), true, false);
                return true;
            }
        } else if (id == R.id.open_in_chrome_id) {
            String url = getTabModelSelector().getCurrentTab().getUrl();
            Intent chromeIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            chromeIntent.setPackage(getApplicationContext().getPackageName());
            chromeIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(chromeIntent);
            return true;
        } else if (id == R.id.find_in_page_id) {
            mFindToolbarManager.showToolbar();
            return true;
        }
        return super.onMenuOrKeyboardAction(id, fromMenu);
    }
}
