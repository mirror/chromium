// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

/**
 * Heuristic for Tap suppression on small text.
 * Handles logging of results seen and activation.
 */
public class SmallTextSuppression extends ContextualSearchHeuristic {
    private static final int DEFAULT_SMALL_TEXT_SIZE = 20;

    private final boolean mIsConditionSatisfied;
    private final int mExperiementThresholdDps;
    private final int mTextSizeDps;

    /**
     * Constructs a Tap suppression heuristic that can suppress a Tap on small text.
     * This logs activation data that includes whether it activated for a threshold specified
     * by an experiment. This also logs Results-seen data to profile when results are seen relative
     * to the text being small or not small.
     * TODO(donnd): include Zoom information so we can tell when small text has been zoomed to big!
     * @param textHeight the height of the text in pixels.
     */
    SmallTextSuppression(int textHeight) {
        assert textHeight > 0;
        mTextSizeDps = textHeight;
        mExperiementThresholdDps = ContextualSearchFieldTrial.getSmallTextSizeLimitDps();
        int sizeLimit =
                mExperiementThresholdDps > 0 ? mExperiementThresholdDps : DEFAULT_SMALL_TEXT_SIZE;
        mIsConditionSatisfied = textHeight <= sizeLimit;
    }

    @Override
    protected boolean isConditionSatisfiedAndEnabled() {
        return mIsConditionSatisfied && mExperiementThresholdDps > 0;
    }

    @Override
    protected void logResultsSeen(boolean wasSearchContentViewSeen, boolean wasActivatedByTap) {
        if (wasActivatedByTap) {
            ContextualSearchUma.logTextSizeTapped(wasSearchContentViewSeen, mTextSizeDps);
        }
    }

    @Override
    protected boolean shouldAggregateLogForTapSuppression() {
        return true;
    }

    @Override
    protected boolean isConditionSatisfiedForAggregateLogging() {
        return mIsConditionSatisfied;
    }

    // TODO(donnd): log to Ranker!
}
