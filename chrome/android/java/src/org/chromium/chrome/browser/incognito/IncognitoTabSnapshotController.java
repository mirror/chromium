// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.incognito;

import android.view.Window;
import android.view.WindowManager;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerChrome;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * This is the observer that prevents incognito tabs from being visible in Android's app switcher.
 */
public class IncognitoTabSnapshotController
        extends EmptyTabModelSelectorObserver implements OverviewModeObserver {
    private Window mWindow;
    private TabModelSelector mTabModelSelector;
    private boolean mInOverviewMode;

    /**
     * Creates and registers a new {@link IncognitoTabSnapshotController}.
     * @param window The {@link Window} containing the flags to which the secure flag will be added
                     and cleared.
     * @param layoutManager The {@link LayoutManagerChrome} where this observer will be added.
     * @param tabModelSelector The {@link TabModelSelector} from where tab information will be
                               extracted.
     */
    public static void createIncognitoTabSnapshotController(
            Window window, LayoutManagerChrome layoutManager, TabModelSelector tabModelSelector) {
        IncognitoTabSnapshotController observer =
                new IncognitoTabSnapshotController(window, tabModelSelector);
        layoutManager.addOverviewModeObserver(observer);
        tabModelSelector.addObserver(observer);
    }

    @VisibleForTesting
    IncognitoTabSnapshotController(Window window, TabModelSelector tabModelSelector) {
        mWindow = window;
        mTabModelSelector = tabModelSelector;
    }

    @Override
    public void onOverviewModeStartedShowing(boolean showToolbar) {
        mInOverviewMode = true;
        updateIncognitoState();
    }

    @Override
    public void onOverviewModeFinishedShowing() {}

    @Override
    public void onOverviewModeStartedHiding(boolean showToolbar, boolean delayAnimation) {
        mInOverviewMode = false;
    }

    @Override
    public void onOverviewModeFinishedHiding() {}

    @Override
    public void onChange() {
        updateIncognitoState();
    }

    /**
     * Sets the attributes flags to secure if there is an incognito tab visible.
     */
    @VisibleForTesting
    void updateIncognitoState() {
        WindowManager.LayoutParams attributes = mWindow.getAttributes();
        boolean currentSecureState = (attributes.flags & WindowManager.LayoutParams.FLAG_SECURE)
                == WindowManager.LayoutParams.FLAG_SECURE;
        boolean expectedSecureState = isShowingIncognito();
        if (currentSecureState == expectedSecureState) return;

        if (expectedSecureState) {
            mWindow.addFlags(WindowManager.LayoutParams.FLAG_SECURE);
        } else {
            mWindow.clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
        }
    }

    /**
     * @return Whether an incognito tab is visible.
     */
    @VisibleForTesting
    boolean isShowingIncognito() {
        boolean isInIncognitoModel = mTabModelSelector.getCurrentModel().isIncognito();
        // Chrome Home is in overview mode when creating new tabs.
        if (mInOverviewMode) {
            return isInIncognitoModel || mTabModelSelector.getModel(true).getCount() != 0;
        }
        return isInIncognitoModel;
    }

    // Set in overview mode for testing.
    @VisibleForTesting
    public void setInOverViewMode(boolean overviewMode) {
        mInOverviewMode = overviewMode;
    }
}
