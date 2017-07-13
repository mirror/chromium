// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.v7.widget.RecyclerView;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.ui.mojom.WindowOpenDisposition;

/**
 * ViewHolder used to show contextual suggestions. The card will be used in the
 * {@link SuggestionsCarouselAdapter} to populate the {@link SuggestionsCarousel} in Chrome Home in
 * the bottom sheet.
 */

public class ContextualSuggestionsCardViewHolder extends NewTabPageViewHolder {
    private SuggestionsUiDelegate mUiDelegate;
    private SnippetArticle mArticle;
    private SuggestionsBinder mSuggestionsBinder;

    public ContextualSuggestionsCardViewHolder(
            RecyclerView recyclerView, SuggestionsUiDelegate uiDelegate) {
        super(LayoutInflater.from(recyclerView.getContext())
                        .inflate(R.layout.contextual_suggestions_card, recyclerView, false));

        mUiDelegate = uiDelegate;
        mSuggestionsBinder = new SuggestionsBinder(itemView, uiDelegate);

        // Set the card width to be less than the the smaller dimension of the screen.
        ViewGroup.LayoutParams params =
                itemView.findViewById(R.id.contextual_card_view).getLayoutParams();
        params.width = getScreenWidthPx() * 9 / 10;
        itemView.findViewById(R.id.contextual_card_view).setLayoutParams(params);

        itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SuggestionsMetrics.recordCardTapped();
                int windowDisposition = WindowOpenDisposition.CURRENT_TAB;
                mUiDelegate.getEventReporter().onSuggestionOpened(
                        mArticle, windowDisposition, mUiDelegate.getSuggestionsRanker());
                mUiDelegate.getNavigationDelegate().openSnippet(windowDisposition, mArticle);
            }
        });
    }

    public void onBindViewHolder(SnippetArticle article) {
        mArticle = article;
        updateLayout();
        mSuggestionsBinder.updateViewInformation(mArticle);
    }

    /**
     * Updates the layout taking into account screen dimensions and the type of snippet displayed.
     */
    private void updateLayout() {
        boolean showHeadline = shouldShowHeadline();
        boolean showDescription = shouldShowDescription(/* unused */ 0);
        boolean showThumbnail = shouldShowThumbnail(/* unused */ 0);
        boolean showThumbnailVideoOverlay = shouldShowThumbnailVideoOverlay(showThumbnail);

        mSuggestionsBinder.updateFieldsVisibility(
                showHeadline, showDescription, showThumbnail, showThumbnailVideoOverlay, 3);
    }

    private boolean shouldShowHeadline() {
        return true;
    }

    private boolean shouldShowDescription(int layout) {
        return false;
    }

    private boolean shouldShowThumbnail(int layout) {
        return true;
    }

    private boolean shouldShowThumbnailVideoOverlay(boolean showThumbnail) {
        if (!showThumbnail) return false;
        if (!mArticle.mIsVideoSuggestion) return false;
        return ChromeFeatureList.isEnabled(ChromeFeatureList.CONTENT_SUGGESTIONS_VIDEO_OVERLAY);
    }

    private int getScreenWidthPx() {
        DisplayMetrics displayMetrics = itemView.getContext().getResources().getDisplayMetrics();

        int screenWidthDp =
                itemView.getContext().getResources().getConfiguration().smallestScreenWidthDp;
        int screenWidthPx =
                Math.round(screenWidthDp * (displayMetrics.xdpi / DisplayMetrics.DENSITY_DEFAULT));

        return screenWidthPx;
    }
}