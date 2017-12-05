// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tabmodel;

import android.view.Window;
import android.view.WindowManager;

import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.chrome.browser.tab.Tab;

/**
 * This is the observer that prevents incognito tabs from being visible in Android's app switcher.
 */
public class TabModelIncognitoObserver implements TabModelSelectorObserver, OverviewModeObserver {
    private ChromeTabbedActivity mTabbedActivity;
    private boolean mInOverviewMode;

    public TabModelIncognitoObserver(ChromeTabbedActivity chromeTabbedActivity) {
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
        setAttributesFlag(mTabbedActivity.getActivityTab().isIncognito());
    }

    @Override
    public void onNewTabCreated(Tab tab) {}

    @Override
    public void onTabModelSelected(TabModel newModel, TabModel oldModel) {}

    @Override
    public void onTabStateInitialized() {}

    /**
     * Sets the attributes flags to secure if there is an incognito tab visible.
     * @param setToSecure If the flag should be set to FLAG_SECURE.
     */
    private void setAttributesFlag(boolean setToSecure) {
        if (mTabbedActivity.getActivityTab() == null) return;

        Window tabWindow = mTabbedActivity.getActivityTab().getActivity().getWindow();
        WindowManager.LayoutParams attributes = tabWindow.getAttributes();
        attributes.flags = 0;
        if (setToSecure) attributes.flags |= WindowManager.LayoutParams.FLAG_SECURE;
        tabWindow.setAttributes(attributes);
    }

    /**
     * @return The number of incognito tabs open.
     */
    private boolean hasIncognitoTab() {
        return mTabbedActivity.getTabModelSelector().getModel(true).getCount() != 0;
    }
}
