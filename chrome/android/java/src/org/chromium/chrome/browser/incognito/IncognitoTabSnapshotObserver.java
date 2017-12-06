// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.incognito;

import android.view.Window;
import android.view.WindowManager;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerChrome;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * This is the observer that prevents incognito tabs from being visible in Android's app switcher.
 */
public class IncognitoTabSnapshotObserver
        extends EmptyTabModelSelectorObserver implements OverviewModeObserver {
    private ChromeTabbedActivity mTabbedActivity;
    private boolean mInOverviewMode;

    private IncognitoTabSnapshotObserver(ChromeTabbedActivity chromeTabbedActivity) {
        mTabbedActivity = chromeTabbedActivity;
    }

    @Override
    public void onOverviewModeStartedShowing(boolean showToolbar) {
        mInOverviewMode = true;
        setAttributesFlag(hasIncognitoTab());
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
        if (mInOverviewMode) {
            setAttributesFlag(hasIncognitoTab());
            return;
        }
        setAttributesFlag(mTabbedActivity.getCurrentTabModel().isIncognito());
    }

    /**
     * Sets the attributes flags to secure if there is an incognito tab visible.
     * @param setToSecure If the flag should be set to FLAG_SECURE.
     */
    private void setAttributesFlag(boolean setToSecure) {
        Window tabWindow = mTabbedActivity.getWindow();
        WindowManager.LayoutParams attributes = tabWindow.getAttributes();
        tabWindow.clearFlags(WindowManager.LayoutParams.FLAG_SECURE);

        if (setToSecure && !ChromeFeatureList.isEnabled(ChromeFeatureList.INCOGNITO_SNAPSHOTS)) {
            attributes.flags |= WindowManager.LayoutParams.FLAG_SECURE;
        }
        tabWindow.setAttributes(attributes);
    }

    /**
     * @return Whether there is an incognito tab open.
     */
    private boolean hasIncognitoTab() {
        return mTabbedActivity.getTabModelSelector().getModel(true).getCount() != 0;
    }

    /**
     * Creates and registers a new {@link IncognitoTabSnapshotObserver}.
     * @param activity The {@link ChromeTabbedActivity} to check the status of tabs.
     * @param layoutManager The {@link LayoutManagerChrome} where this observer will be added.
     * @param tabModelSelector The {@link TabModelSelector} where this observer will be added.
     */
    public static void registerNewObserver(ChromeTabbedActivity activity,
            LayoutManagerChrome layoutManager, TabModelSelector tabModelSelector) {
        IncognitoTabSnapshotObserver observer = new IncognitoTabSnapshotObserver(activity);
        layoutManager.addOverviewModeObserver(observer);
        tabModelSelector.addObserver(observer);
    }
}
