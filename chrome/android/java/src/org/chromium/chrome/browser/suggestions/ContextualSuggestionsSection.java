// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.webkit.URLUtil;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.metrics.OneShotImpressionListener;
import org.chromium.chrome.browser.ntp.cards.InnerNode;
import org.chromium.chrome.browser.ntp.cards.NewTabPageAdapter;
import org.chromium.chrome.browser.ntp.cards.SuggestionsCategoryInfo;
import org.chromium.chrome.browser.ntp.cards.SuggestionsSection;
import org.chromium.chrome.browser.ntp.snippets.ContentSuggestionsCardLayout;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.ui.widget.Toast;

import java.util.Locale;

/**
 * A prototype section for contextual suggestions.
 */
public class ContextualSuggestionsSection extends InnerNode implements SuggestionsSection.Delegate {
    private NewTabPageAdapter mAdapter;
    private final SuggestionsUiDelegate mUiDelegate;
    private final OneShotImpressionListener mImpressionFilter;
    private final OfflinePageBridge mOfflinePageBridge;
    @Nullable
    private String mCurrentContextUrl;

    SuggestionsSection mSection;

    public ContextualSuggestionsSection(
            SuggestionsUiDelegate uiDelegate, OfflinePageBridge offlinePageBridge) {
        mUiDelegate = uiDelegate;
        mOfflinePageBridge = offlinePageBridge;

        // The impression tracker will record metrics only once per bottom sheet opened.
        mImpressionFilter = new OneShotImpressionListener(
                SuggestionsMetrics::recordContextualSuggestionsCarouselShown);

        SuggestionsCategoryInfo info = new SuggestionsCategoryInfo(KnownCategories.CONTEXTUAL,
                "People also viewed", ContentSuggestionsCardLayout.FULL_CARD,
                ContentSuggestionsAdditionalAction.NONE, false, "");
        mSection = new SuggestionsSection(
                this, mUiDelegate, mUiDelegate.getSuggestionsRanker(), mOfflinePageBridge, info);
        mUiDelegate.getSuggestionsRanker().registerCategory(KnownCategories.CONTEXTUAL);
        addChild(mSection);
    }

    public void setNewTabPageAdapter(NewTabPageAdapter adapter) {
        mAdapter = adapter;
    }

    /**
     * Fetches new suggestions if the context URL was changed from the last time that the carousel
     * was shown.
     */
    public void refresh(final Context context, @Nullable final String newUrl) {
        if (!URLUtil.isNetworkUrl(newUrl)) {
            clearSuggestions();
            return;
        }

        // Reset the impression tracker to record if the carousel is shown. We do this once per
        // bottom sheet opened.
        mImpressionFilter.reset();

        // Do nothing if there are already suggestions in the carousel for the new context.
        if (isContextTheSame(newUrl)) return;

        String text = "Fetching contextual suggestions...";
        Toast.makeText(context, text, Toast.LENGTH_SHORT).show();

        // Context has changed, so we want to remove any old suggestions from the carousel.
        clearSuggestions();
        mCurrentContextUrl = newUrl;

        mUiDelegate.getSuggestionsSource().fetchContextualSuggestions(newUrl, (suggestions) -> {
            // Avoiding double fetches causing suggestions for incorrect context.
            if (!TextUtils.equals(newUrl, mCurrentContextUrl)) return;

            if (suggestions.size() > 0) {
                mSection.appendSuggestions(suggestions, false, false);
                mAdapter.forceShowArticlesHeader();
                mSection.setHeaderVisibility(true);
            }

            String toastText = String.format(
                    Locale.US, "Fetched %d contextual suggestions.", suggestions.size());
            Toast.makeText(context, toastText, Toast.LENGTH_SHORT).show();
        });
    }

    @VisibleForTesting
    boolean isContextTheSame(String newUrl) {
        // The call to UrlUtilities is wrapped to be able to mock it and skip the native call in
        // unit tests.
        return UrlUtilities.urlsMatchIgnoringFragments(newUrl, mCurrentContextUrl);
    }

    /**
     * Removes any suggestions that might be present in the carousel.
     */
    private void clearSuggestions() {
        mCurrentContextUrl = null;
        mSection.clearData();
        mSection.setHeaderVisibility(false);
    }

    @Override
    public void dismissSection(SuggestionsSection section) {
        // TODO(twellington): Auto-generated method stub
    }

    @Override
    public boolean isResetAllowed() {
        // TODO(twellington): Auto-generated method stub
        return false;
    }
}
