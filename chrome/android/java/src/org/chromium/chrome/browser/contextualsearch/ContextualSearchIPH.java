// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.graphics.Rect;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.bottombar.contextualsearch.ContextualSearchPanel;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.widget.textbubble.TextBubble;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;

/**
 * Helper class for displaying In-Product Help UI for Contextual Search.
 */
class ContextualSearchIPH {
    private final Profile mProfile;
    private final ContextualSearchPanel mSearchPanel;
    private final String mFeatureName;
    private final int mStringId;

    private TextBubble mHelpBubble;
    private boolean mIsShowing;

    /**
     * Constructs the helper class and initializes string ids according to featureName.
     * @param featureName Name of the feature in IPH, look at {@link FeatureConstants}.
     * @param searchPanel The instance of {@link ContextualSearchPanel}
     * @param profile The {@link Profile} used for {@link TrackerFactory}.
     */
    ContextualSearchIPH(String featureName, ContextualSearchPanel searchPanel, Profile profile) {
        mFeatureName = featureName;
        mSearchPanel = searchPanel;
        mProfile = profile;
        if (featureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)) {
            mStringId = R.string.contextual_search_iph_entity;
        } else {
            mStringId = R.string.contextual_search_iph_search_result;
        }
    }

    /**
     * Shows the appropriate In-Product Help UI if the conditions are met.
     * @param parentView The parent view that the bubble is attached to.
     */
    void maybeShow(View parentView) {
        if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE)) {
            maybeShowBubbleAbovePanel(mSearchPanel, parentView);
        } else if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
            Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
            if (tracker.shouldTriggerHelpUI(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
                mSearchPanel.showBarBanner();
                mIsShowing = true;
            }
        }
    }

    /**
     * Shows a help bubble above the Contextual Search panel if the In-Product Help conditions are
     * met.
     * @param searchPanel The instance of {@link ContextualSearchPanel}.
     * @param parentView The parent view that the bubble is attached to.
     */
    private void maybeShowBubbleAbovePanel(ContextualSearchPanel searchPanel, View parentView) {
        if (searchPanel == null || !searchPanel.isShowing() || parentView == null) return;

        assert(mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE));

        final Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
        if (!tracker.shouldTriggerHelpUI(mFeatureName)) return;

        if (mHelpBubble != null) {
            mHelpBubble.dismiss();
        }
        mHelpBubble = new TextBubble(parentView.getContext(), parentView, mStringId, mStringId);
        Rect anchorRect = searchPanel.getPanelRect();
        int yInsetPx = parentView.getResources().getDimensionPixelOffset(
                R.dimen.contextual_search_bubble_y_inset);
        anchorRect.top += yInsetPx;
        mHelpBubble.setAnchorRect(anchorRect);

        mHelpBubble.setDismissOnTouchInteraction(true);
        mHelpBubble.addOnDismissListener(() -> {
            tracker.dismissed(mFeatureName);
            mIsShowing = false;
        });

        mHelpBubble.show();
        mIsShowing = true;
    }

    /**
     * Dismisses the In-Product Help UI.
     */
    void dismiss() {
        if (!mIsShowing) return;

        if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE)) {
            mHelpBubble.dismiss();
        } else if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
            Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
            tracker.dismissed(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE);
            mSearchPanel.hideBarBanner();
        }
        mIsShowing = false;
    }

    /**
     * @return Whether the In-Product Help UI is currently showing.
     */
    boolean isShowing() {
        return mIsShowing;
    }
}
