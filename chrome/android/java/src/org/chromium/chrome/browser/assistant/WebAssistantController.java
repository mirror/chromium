// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import android.graphics.drawable.Drawable;
import android.view.View.OnClickListener;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.devtools.DevToolsAgentHost;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;
import org.chromium.content_public.browser.WebContents;

import java.util.ArrayList;

/** Main controller for tabs that are assisted by the Web Assistant. */
public class WebAssistantController {
    private final WebAssistantDelegate mDelegate;
    private final WebAssistantBottomBarController mBottomBarController;
    private Tab mTab;
    private TabObserver mTabObserver;
    private GestureStateListener mGestureListener;
    private DevToolsAgentHost mHost;
    private WebAssistantService mService;
    private ArrayList<WebAssistantAction> mCurrentActions;
    private WebAssistantAction mCurrentAction;
    private String mCurrentUrl;
    private boolean mCheckActionsInProgress;

    public WebAssistantController(WebAssistantDelegate delegate) {
        mDelegate = delegate;
        mBottomBarController = new WebAssistantBottomBarController(delegate.getActivity());
        mCheckActionsInProgress = false;
        showAssistantIcon();
    }

    public void onTabReady() {
        mTab = mDelegate.getActivity().getActivityTab();
        if (mTab == null) return;

        WebContents webContents = mTab.getWebContents();
        if (webContents == null) {
            mTab = null;
            return;
        }

        mHost = mTab.getOrCreateDevToolsAgentHost();
        if (mHost == null) {
            mTab = null;
            return;
        }

        mBottomBarController.onDevToolAgentHostReady(mHost);

        mTabObserver = makeTabObserver();
        mTab.addObserver(mTabObserver);

        mGestureListener = makeGestureListener();
        GestureListenerManager.fromWebContents(webContents).addListener(mGestureListener);

        mService = new WebAssistantService(mDelegate.getWebAssistantServiceUrl());

        if (!mTab.isLoading()) {
            onTabReadyForAssistance();
        }
    }

    public void close() {
        if (mTab != null) {
            if (mGestureListener != null) {
                WebContents webContents = mTab.getWebContents();
                if (webContents != null) {
                    GestureListenerManager.fromWebContents(webContents)
                            .removeListener(mGestureListener);
                }
                mGestureListener = null;
            }
            if (mTabObserver != null) {
                mTab.removeObserver(mTabObserver);
                mTabObserver = null;
            }
            mTab = null;
        }
        if (mHost != null) {
            mHost.close();
            mHost = null;
        }
        if (mService != null) {
            mService.close();
            mService = null;
        }
    }

    private void onAssistantIconClicked() {
        mBottomBarController.toggle();
    }

    private void onTabReadyForAssistance() {
        if (mTab == null || mHost == null || mService == null) return;
        String url = mTab.getUrl();
        if (url == null || "".equals(url)) return;
        if (mCurrentUrl != null && mCurrentUrl.equals(url)) return;
        invalidateCurrentActions();
        mCurrentUrl = url;
        android.util.Log.e("foo", "-- checking for URL: " + url);
        mService.getActionsForUrl(url, this ::onActionsForUrl);
    }

    private void onActionsForUrl(String url, ArrayList<WebAssistantAction> actions) {
        android.util.Log.e(
                "foo", "-- onActionsForUrl: " + url + ", found " + actions.size() + " actions");
        if (!url.equals(mCurrentUrl)) return;
        invalidateCurrentActions();
        mCurrentActions = actions;
        checkActions();
    }

    private void invalidateCurrentActions() {
        mBottomBarController.hide();
        mBottomBarController.clear();
        mCurrentAction = null;
        mCurrentActions = null;
    }

    private void checkActions() {
        // If an action is already being shown then don't check again.
        // Don't start a new check if a check is in progress too.
        if (mTab == null || mHost == null || mCurrentActions == null || mCurrentAction != null
                || mCheckActionsInProgress) {
            return;
        }

        // If the page is scrolling then don't check yet.
        if (mTab.getContentViewCore().isScrollInProgress()) return;

        android.util.Log.e(
                "foo", "-- check current actions now, candidates: " + mCurrentActions.size());
        mCheckActionsInProgress = true;

        // TODO
    }

    private void showAssistantIcon() {
        // Show an Assistant logo instead of the usual custom action in the toolbar when the
        // Web Assistant is enabled.
        // TODO: replace googleg with an animated Assistant drawable.
        Drawable icon =
                ApiCompatibilityUtils.getDrawable(mDelegate.getResources(), R.drawable.googleg);
        String description = "Assistant";
        OnClickListener listener = (view) -> {
            onAssistantIconClicked();
        };
        mDelegate.getToolbarManager().setCustomActionButton(icon, description, listener);
    }

    private TabObserver makeTabObserver() {
        return new EmptyTabObserver() {
            @Override
            public void onDestroyed(Tab tab) {
                close();
            }
            @Override
            public void onPageLoadFinished(Tab tab) {
                onTabReadyForAssistance();
            }
            @Override
            public void onPageLoadStarted(Tab tab, String url) {
                invalidateCurrentActions();
            }
        };
    }

    private GestureStateListener makeGestureListener() {
        return new GestureStateListener() {
            @Override
            public void onPinchEnded() {
                checkActions();
            }
            @Override
            public void onFlingEndGesture(int scrollOffsetY, int scrollExtentY) {
                checkActions();
            }
            @Override
            public void onScrollEnded(int scrollOffsetY, int scrollExtentY) {
                checkActions();
            }
            @Override
            public void onScrollOffsetOrExtentChanged(int scrollOffsetY, int scrollExtentY) {
                checkActions();
            }
            @Override
            public void onSingleTap(boolean consumed) {
                checkActions();
            }
            @Override
            public void onLongPress() {
                checkActions();
            }
        };
    }
}
