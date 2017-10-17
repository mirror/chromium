// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.LinearLayout;

import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;

/**
 * A menu header helping users find where their content moved when Chrome Home is enabled.
 * The Chrome Home in-product help bubble pointing toward the bottom sheet is displayed on click.
 */
public class ChromeHomeIphMenuHeader extends LinearLayout implements OnClickListener {
    private ChromeActivity mActivity;
    private boolean mDismissChromeHomeIPHOnMenuDismissed;

    public ChromeHomeIphMenuHeader(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Initializes the menu header.
     * @param activity The {@link ChromeActivity} that will display the app menu containing this
     *                 header.
     */
    public void initialize(ChromeActivity activity) {
        mActivity = activity;
        mDismissChromeHomeIPHOnMenuDismissed = true;
        setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        mDismissChromeHomeIPHOnMenuDismissed = false;

        Tracker tracker = TrackerFactory.getTrackerForProfile(Profile.getLastUsedProfile());
        tracker.notifyEvent(EventConstants.CHROME_HOME_MENU_HEADER_CLICKED);

        mActivity.getBottomSheet().getBottomSheetMetrics().recordInProductHelpMenuItemClicked();
        mActivity.getBottomSheet().showHelpBubble(true);

        mActivity.getAppMenuHandler().hideAppMenu();
    }

    /**
     * Must be called when the app menu is dismissed. Used to notify the IPH system that the menu
     * header is no longer showing.
     */
    public void onMenuDismissed() {
        if (!mDismissChromeHomeIPHOnMenuDismissed) return;

        Tracker tracker = TrackerFactory.getTrackerForProfile(Profile.getLastUsedProfile());
        tracker.dismissed(FeatureConstants.CHROME_HOME_MENU_HEADER_FEATURE);
        mDismissChromeHomeIPHOnMenuDismissed = false;
    }
}
