// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.ComponentName;
import android.content.Intent;
import android.provider.Browser;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.layouts.LayoutManager;
import org.chromium.chrome.browser.fullscreen.ChromeFullscreenManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabReparentingParams;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.widget.ControlContainer;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * An Activity used to display fullscreen WebContents.
 *
 * This Activity used to be called FullscreenWebContentsActivity and extended FullScreenActivity.
 * When FullScreenActivity was renamed to SingleTabActivity, this was changed to FullscreenActivity.
 */
public class FullscreenActivity extends SingleTabActivity {
    private static final String TAG = "FullscreenActivity";

    private static final SparseArray<Tab> sTabsToSteal = new SparseArray<>();

    private WebContentsObserver mWebContentsObserver;

    @Override
    protected Tab createTab() {
        assert getIntent().hasExtra(IntentHandler.EXTRA_TAB_ID);

        int tabId = IntentUtils.safeGetIntExtra(
                getIntent(), IntentHandler.EXTRA_TAB_ID, Tab.INVALID_TAB_ID);

        Log.d("Peter", "Stealing");
        final Tab tab = sTabsToSteal.get(tabId);
        sTabsToSteal.remove(tabId);
        assert tab != null;

        tab.sever();
        tab.attachAndFinishReparenting(this, createTabDelegateFactory(),
                new TabReparentingParams(null, null, null));

        tab.getFullscreenManager().setTab(tab);
        tab.toggleFullscreenMode(true);

        mWebContentsObserver = new WebContentsObserver(tab.getWebContents()) {
            @Override
            public void didFinishNavigation(String url, boolean isInMainFrame, boolean isErrorPage,
                    boolean hasCommitted, boolean isSameDocument, boolean isFragmentNavigation,
                    Integer pageTransition, int errorCode, String errorDescription,
                    int httpStatusCode) {
                if (hasCommitted && isInMainFrame) {
                    // Notify the renderer to permanently hide the top controls since they do
                    // not apply to fullscreen content views.
                    tab.updateBrowserControlsState(tab.getBrowserControlsStateConstraints(), true);
                }
            }
        };
        return tab;
    }

    @Override
    public void finishNativeInitialization() {
        ControlContainer controlContainer = (ControlContainer) findViewById(R.id.control_container);
        initializeCompositorContent(new LayoutManager(getCompositorViewHolder()),
                (View) controlContainer, (ViewGroup) findViewById(android.R.id.content),
                controlContainer);

        if (getFullscreenManager() != null) getFullscreenManager().setTab(getActivityTab());
        super.finishNativeInitialization();
    }

    @Override
    protected void initializeToolbar() {}

    @Override
    protected int getControlContainerLayoutId() {
        // TODO(peconn): Determine if there's something more suitable to use here.
        return R.layout.webapp_control_container;
    }

    @Override
    protected ChromeFullscreenManager createFullscreenManager() {
        // Create a Fullscreen manager that won't change the Tab's fullscreen state when the
        // Activity ends - we handle leaving fullscreen ourselves.
        return new ChromeFullscreenManager(this, false, false);
    }

    @Override
    public boolean supportsFullscreenActivity() {
        return true;
    }

    public static void toggleFullscreenMode(final boolean enableFullscreen, final Tab tab) {
        Log.d("Peter", "toggleFullscreenMode()");
        if (tab.getFullscreenManager() == null) {
            Log.w(TAG, "Cannot toggle fullscreen, manager is null.");
            return;
        }

        if (tab.getFullscreenManager().getTab() == tab) {
            tab.getFullscreenManager().setTab(null);
        }

        if (enableFullscreen) {
            launchFullscreenActivityThenStealTab(tab);
        } else {
            reparentTabToOriginalOwner(tab);
        }
    }

    private static void reparentTabToOriginalOwner(final Tab tab) {
        ChromeActivity activity = tab.getActivity();

        activity.exitFullscreenIfShowing();

        Runnable exitFullscreen = () -> {
            // The Tab's FullscreenManager changes when it is moved.
            tab.getFullscreenManager().setTab(tab);
            tab.toggleFullscreenMode(false);
        };


        Intent intent = new Intent();

        // Send back to the Activity it came from.
        ComponentName parent = IntentUtils.safeGetParcelableExtra(
                activity.getIntent(), IntentHandler.EXTRA_PARENT_COMPONENT);

        // By default Intents from Chrome open in the current tab. We add this extra to prevent
        // clobbering the top tab.
        intent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);

        if (parent != null) {
            intent.setComponent(parent);
        } else {
            Log.d(TAG, "Cannot return fullscreen tab to parent Activity.");
            // Tab.detachAndStartReparenting will give the intent a default component if it
            // has none.
        }

        ChromeActivity tabActivity = tab.getActivity();
        if (tabActivity instanceof FullscreenActivity) {
            FullscreenActivity fullscreenActivity = (FullscreenActivity) tabActivity;
            if (fullscreenActivity.mWebContentsObserver != null) {
                fullscreenActivity.mWebContentsObserver.destroy();
                fullscreenActivity.mWebContentsObserver = null;
            }
        }

        intent.putExtra(Browser.EXTRA_APPLICATION_ID, activity.getPackageName());

        tab.detachAndStartReparenting(intent, null, exitFullscreen);
    }

    private static void launchFullscreenActivityThenStealTab(Tab tab) {
        sTabsToSteal.put(tab.getId(), tab);

        ChromeActivity activity = tab.getActivity();
        Intent intent = new Intent();

        intent.putExtra(IntentHandler.EXTRA_TAB_ID, tab.getId());

        // Send to the FullscreenActivity.
        intent.setClass(activity, FullscreenActivity.class);

        intent.putExtra(IntentHandler.EXTRA_PARENT_COMPONENT, activity.getComponentName());
        // In multiwindow mode we want both activities to be able to launch independent
        // FullscreenActivity's.
        intent.addFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, activity.getPackageName());

        activity.startActivity(intent);
    }

}
