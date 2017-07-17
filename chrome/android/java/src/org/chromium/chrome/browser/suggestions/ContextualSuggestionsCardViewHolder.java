// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.widget.displaystyle.HorizontalDisplayStyle;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;
import org.chromium.ui.mojom.WindowOpenDisposition;

/**
 * ViewHolder used to show contextual suggestions. The card will be used in the
 * {@link SuggestionsCarouselAdapter} to populate the {@link SuggestionsCarousel} in Chrome Home in
 * the bottom sheet.
 */

public class ContextualSuggestionsCardViewHolder extends NewTabPageViewHolder {
    private static final double CARD_WIDTH_TO_WINDOW_SIZE_RATIO = 0.9;
    private final UiConfig mUiConfig;
    private SuggestionsUiDelegate mUiDelegate;
    private SnippetArticle mArticle;
    private SuggestionsBinder mSuggestionsBinder;

    public ContextualSuggestionsCardViewHolder(
            ViewGroup recyclerView, UiConfig uiConfig, SuggestionsUiDelegate uiDelegate) {
        super(LayoutInflater.from(recyclerView.getContext())
                        .inflate(R.layout.contextual_suggestions_card, recyclerView, false));

        mUiConfig = uiConfig;
        mUiDelegate = uiDelegate;
        mSuggestionsBinder = new SuggestionsBinder(itemView, uiDelegate);

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

        updateCardWidth();

        mSuggestionsBinder.updateFieldsVisibility(
                showHeadline, showDescription, showThumbnail, showThumbnailVideoOverlay, 3);
    }

    private void updateCardWidth() {
        final int horizontalStyle = mUiConfig.getCurrentDisplayStyle().horizontal;

        // Set the card width to be less than the the smaller dimension of the screen.
        ViewGroup.LayoutParams params = itemView.getLayoutParams();
        DisplayMetrics displayMetrics = itemView.getContext().getResources().getDisplayMetrics();

        // Size of the dimension of the window which we will use to calculate the width of the card.
        int screenSizePx = horizontalStyle == HorizontalDisplayStyle.WIDE
                ? displayMetrics.heightPixels
                : displayMetrics.widthPixels;

        params.width = (int) (screenSizePx * CARD_WIDTH_TO_WINDOW_SIZE_RATIO);
        itemView.setLayoutParams(params);
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
}