// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.os.SystemClock;
import android.support.v7.widget.RecyclerView;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.ntp.snippets.FaviconFetchResult;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;

import java.util.concurrent.TimeUnit;

/**
 * Exposes methods to report suggestions related events, for UMA or Fetch scheduling purposes.
 */
public abstract class SuggestionsMetrics {
    private SuggestionsMetrics() {}

    // UI Element interactions

    public static void recordSurfaceVisible() {
        if (!ChromePreferenceManager.getInstance().getSuggestionsSurfaceShown()) {
            RecordUserAction.record("Suggestions.FirstTimeSurfaceVisible");
            ChromePreferenceManager.getInstance().setSuggestionsSurfaceShown();
        }

        RecordUserAction.record("Suggestions.SurfaceVisible");
    }

    public static void recordSurfaceHalfVisible() {
        RecordUserAction.record("Suggestions.SurfaceHalfVisible");
    }

    public static void recordSurfaceFullyVisible() {
        RecordUserAction.record("Suggestions.SurfaceFullyVisible");
    }

    public static void recordSurfaceHidden() {
        RecordUserAction.record("Suggestions.SurfaceHidden");
    }

    public static void recordTileTapped() {
        RecordUserAction.record("Suggestions.Tile.Tapped");
    }

    public static void recordCardTapped() {
        RecordUserAction.record("Suggestions.Card.Tapped");
    }

    public static void recordCardActionTapped() {
        RecordUserAction.record("Suggestions.Card.ActionTapped");
    }

    public static void recordCardSwipedAway() {
        RecordUserAction.record("Suggestions.Card.SwipedAway");
    }

    // Effect/Purpose of the interactions. Most are recorded in |content_suggestions_metrics.h|

    public static void recordActionViewAll() {
        RecordUserAction.record("Suggestions.Category.ViewAll");
    }

    static void recordFaviconFetchTime(long faviconFetchStartTimeMs) {
        RecordHistogram.recordMediumTimesHistogram(
                "NewTabPage.ContentSuggestions.ArticleFaviconFetchTime",
                SystemClock.elapsedRealtime() - faviconFetchStartTimeMs, TimeUnit.MILLISECONDS);
    }

    static void recordFaviconFetchResult(SnippetArticle suggestion, @FaviconFetchResult int result,
            long faviconFetchStartTimeMs) {
        // Record the histogram for articles only to have a fair comparision.
        if (!suggestion.isArticle()) return;
        RecordHistogram.recordEnumeratedHistogram(
                "NewTabPage.ContentSuggestions.ArticleFaviconFetchResult", result,
                FaviconFetchResult.COUNT);
        recordFaviconFetchTime(faviconFetchStartTimeMs);
    }

    /**
     * One-shot reporter that records the first time the user scrolls a {@link RecyclerView}. If it
     * should be reused, call {@link #reset()} to rearm it.
     */
    public static class ScrollEventReporter extends RecyclerView.OnScrollListener {
        private boolean mFired;
        @Override
        public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
            if (mFired) return;
            if (newState != RecyclerView.SCROLL_STATE_DRAGGING) return;

            RecordUserAction.record("Suggestions.ScrolledAfterOpen");
            mFired = true;
        }

        public void reset() {
            mFired = false;
        }
    }
}
