// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.bottombar.contextualsearch.ContextualSearchPanel;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.widget.textbubble.TextBubble;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;

/**
 * Helper class for displaying In-Product Help bubbles for Contextual Search.
 */
public class ContextualSearchIPH {
    private TextBubble mHelpBubble;
    private Profile mProfile;
    private String mFeatureName;

    private int mStringId;
    private int mAccessibilityStringId;
    private boolean mIsShowing;

    /**
     * Constructs the helper class and initializes string ids according to featureName.
     * @param featureName Name of the feature in IPH, look at {@link FeatureConstants}.
     * @param profile {@link Profile} used for {@link TrackerFactory}.
     */
    public ContextualSearchIPH(String featureName, Profile profile) {
        mFeatureName = featureName;
        mProfile = profile;
        mStringId = R.string.contextual_search_iph_search_result;
        mAccessibilityStringId = R.string.contextual_search_iph_search_result;
        if (featureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)) {
            mStringId = R.string.contextual_search_iph_entity;
            mAccessibilityStringId = R.string.contextual_search_iph_entity;
        }
    }

    /**
     * Shows the appropriate In-Product Help UI if the conditions are met.
     * @param searchPanel The instance of {@link ContextualSearchPanel}
     * @param parentView The parent view that the bubble is attached to.
     */
    public void maybeShow(ContextualSearchPanel searchPanel, View parentView) {
        if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE)) {
            maybeShowBubbleAbovePanel(searchPanel, parentView);
        } else if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
            Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
            if (tracker.shouldTriggerHelpUI(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
                searchPanel.showBarBanner();
                mIsShowing = true;
            }
        }
    }

    /**
     * Shows a help bubble above the Contextual Search panel if the In-Product Help conditions are
     * met.
     * @param searchPanel The instance of {@link ContextualSearchPanel}
     * @param parentView The parent view that the bubble is attached to.
     */
    private void maybeShowBubbleAbovePanel(ContextualSearchPanel searchPanel, View parentView) {
        if (searchPanel == null || !searchPanel.isShowing()) return;

        if (!mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                && !mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE))
            return;

        final Tracker tracker = TrackerFactory.getTrackerForProfile(Profile.getLastUsedProfile());
        if (!tracker.shouldTriggerHelpUI(mFeatureName)) return;

        if (mHelpBubble != null) {
            mHelpBubble.dismiss();
        }
        mHelpBubble = new TextBubble(
                parentView.getContext(), parentView, mStringId, mAccessibilityStringId);
        mHelpBubble.setAnchorRect(searchPanel.getPanelRect());

        mHelpBubble.setDismissOnTouchInteraction(true);
        mHelpBubble.addOnDismissListener(() -> { tracker.dismissed(mFeatureName); });

        mHelpBubble.show();
        mIsShowing = true;
    }

    /**
     * Dismisses the In-Product Help UI.
     */
    public void dismiss() {
        if (!mIsShowing) return;

        if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE)) {
            mHelpBubble.dismiss();
        } else if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
            Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
            tracker.dismissed(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE);
        }
        mIsShowing = false;
    }

    /**
     * @return Whether the In-Product Help UI is currently showing.
     */
    public boolean isShowing() {
        return mIsShowing;
    }
}
