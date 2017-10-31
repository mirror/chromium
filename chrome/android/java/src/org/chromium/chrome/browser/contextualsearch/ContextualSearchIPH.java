// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.graphics.Rect;
import android.text.TextUtils;
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
    private final View mParentView;

    private ContextualSearchPanel mSearchPanel;
    private TextBubble mHelpBubble;
    private String mFeatureName;
    private boolean mIsShowing;

    /**
     * Constructs the helper class and initializes string ids according to featureName.
     * @param parentView The parent view that the {@link TextBubble} will be attached to.
     * @param profile The {@link Profile} used for {@link TrackerFactory}.
     */
    ContextualSearchIPH(View parentView, Profile profile) {
        mParentView = parentView;
        mProfile = profile;
    }

    /**
     * @param searchPanel The instance of {@link ContextualSearchPanel}.
     */
    void setSearchPanel(ContextualSearchPanel searchPanel) {
        mSearchPanel = searchPanel;
    }

    /**
     * Called before panel show is requested.
     * @param wasActivatedByTap Whether Contextual Search was activated by tapping.
     * @param isTapSupported Whether tapping is supported as a trigger for Contextual Search.
     */
    void beforePanelShown(boolean wasActivatedByTap, boolean isTapSupported) {
        if (!wasActivatedByTap && isTapSupported) {
            maybeShow(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE);
        }
    }

    /**
     * Called after the Contextual Search panel's animation is finished.
     * @param wasActivatedByTap Whether Contextual Search was activated by tapping.
     */
    void onPanelFinishedShowing(boolean wasActivatedByTap) {
        if (!wasActivatedByTap) {
            maybeShow(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE);
        }
    }

    /**
     * Called after entity data is received.
     * @param wasActivatedByTap Whether Contextual Search was activated by tapping.
     */
    void onEntityDataReceived(boolean wasActivatedByTap) {
        if (wasActivatedByTap) {
            maybeShow(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE);
        }
    }

    /**
     * Called when the Contextual Search panel is expanded or maximized.
     */
    void onPanelExpandedOrMaximized() {
        if (!TextUtils.isEmpty(mFeatureName)
                && mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_TAP_FEATURE)) {
            dismiss();
        }
    }

    /**
     * Shows the appropriate In-Product Help UI if the conditions are met.
     * @param featureName Name of the feature in IPH, look at {@link FeatureConstants}.
     */
    private void maybeShow(String featureName) {
        if (mIsShowing) return;

        if (mSearchPanel == null || mParentView == null) return;

        mFeatureName = featureName;
        if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE)) {
            maybeShowBubbleAbovePanel();
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
     */
    private void maybeShowBubbleAbovePanel() {
        if (!mSearchPanel.isShowing()) return;

        assert(mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)
                || mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_FEATURE));

        final Tracker tracker = TrackerFactory.getTrackerForProfile(mProfile);
        if (!tracker.shouldTriggerHelpUI(mFeatureName)) return;

        int stringId = R.string.contextual_search_iph_search_result;
        if (mFeatureName.equals(FeatureConstants.CONTEXTUAL_SEARCH_PANEL_FEATURE)) {
            stringId = R.string.contextual_search_iph_entity;
        }

        assert mHelpBubble == null;
        mHelpBubble = new TextBubble(mParentView.getContext(), mParentView, stringId, stringId);
        Rect anchorRect = mSearchPanel.getPanelRect();

        int yInsetPx = mParentView.getResources().getDimensionPixelOffset(
                R.dimen.contextual_search_bubble_y_inset);
        anchorRect.top -= yInsetPx;
        mHelpBubble.setAnchorRect(anchorRect);

        mHelpBubble.setDismissOnTouchInteraction(true);
        mHelpBubble.addOnDismissListener(() -> {
            tracker.dismissed(mFeatureName);
            mIsShowing = false;
            mHelpBubble = null;
        });

        mHelpBubble.show();
        mIsShowing = true;
    }

    /**
     * Dismisses the In-Product Help UI.
     */
    void dismiss() {
        if (!mIsShowing || TextUtils.isEmpty(mFeatureName)) return;

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
}
