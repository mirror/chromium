// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.ViewGroup;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.cards.ImpressionTracker;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeVisitor;
import org.chromium.chrome.browser.ntp.cards.OptionalLeaf;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;
import org.chromium.ui.widget.Toast;

import java.util.Collections;
import java.util.Locale;

/**
 * This is an optional item in Chrome Home in the bottom sheet. The carousel is a {@link
 * RecyclerView} which is used to display contextual suggestions to the user according to the
 * website they are currently looking at. The carousel uses a {@link SuggestionsCarouselAdapter} to
 * populate information in {@link ContextualSuggestionsCardViewHolder} instances.
 *
 * When there is no context, i.e. the user is on a native page, the carousel will not be shown.
 */
public class SuggestionsCarousel extends OptionalLeaf {
    private final SuggestionsCarouselAdapter mAdapter;
    private final SuggestionsUiDelegate mUiDelegate;
    private ImpressionTracker mImpressionTracker;
    private String mCurrentContextUrl;

    public SuggestionsCarousel(UiConfig uiConfig, SuggestionsUiDelegate uiDelegate) {
        mAdapter = new SuggestionsCarouselAdapter(uiConfig, uiDelegate);
        mUiDelegate = uiDelegate;

        // The impression tracker will record metrics only once per bottom sheet opened.
        mImpressionTracker = new ImpressionTracker(null, () -> {
            SuggestionsMetrics.recordContextualSuggestionsCarouselShown();
            mImpressionTracker.reset(null);
        });

        setVisibilityInternal(true);
    }

    /**
     * Removes any suggestions that might be present in the carousel.
     */
    public void clearSuggestions() {
        mCurrentContextUrl = null;
        mAdapter.setSuggestions(Collections.<SnippetArticle>emptyList());
    }

    /**
     * Fetches new suggestions if necessary.
     */
    public void refresh(final Context context, @Nullable final String newUrl) {
        if (TextUtils.isEmpty(newUrl)) clearSuggestions();

        // Reset the impression tracker to record if the carousel is shown.
        mImpressionTracker.setTriggered(false);

        // Do nothing if there are already suggestions in the carousel for the new context.
        if (TextUtils.equals(newUrl, mCurrentContextUrl)) return;

        String text = "Fetching contextual suggestions...";
        Toast.makeText(context, text, Toast.LENGTH_SHORT).show();

        mUiDelegate.getSuggestionsSource().fetchContextualSuggestions(
                newUrl, (contextualSuggestions) -> {
                    mCurrentContextUrl = newUrl;
                    mAdapter.setSuggestions(contextualSuggestions);

                    String toastText = String.format(Locale.US,
                            "Fetched %d contextual suggestions.", contextualSuggestions.size());
                    Toast.makeText(context, toastText, Toast.LENGTH_SHORT).show();
                });
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        ((RecyclerView) holder.itemView).setAdapter(mAdapter);

        assert mImpressionTracker != null;

        // We want to record only once that the carousel was shown after the bottom sheet was
        // opened. The first time that the carousel is shown, the mImpressionTracker will be
        // triggered and we don't want it to track any views anymore.
        mImpressionTracker.reset(mImpressionTracker.wasTriggered() ? null : holder.itemView);
    }

    @Override
    protected int getItemViewType() {
        return ItemViewType.CAROUSEL;
    }

    @Override
    protected void visitOptionalItem(NodeVisitor visitor) {
        visitor.visitCarouselItem(mAdapter);
    }

    /**
     * View holder for the {@link SuggestionsCarousel}.
     */
    public static class ViewHolder extends NewTabPageViewHolder {
        public ViewHolder(ViewGroup parentView) {
            super(new RecyclerView(parentView.getContext()));

            setUpRecyclerView();
        }

        private void setUpRecyclerView() {
            RecyclerView recyclerView = (RecyclerView) itemView;
            recyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
                @Override
                public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
                    // The dragging state guarantees that the scroll event was caused by the user.
                    if (newState != RecyclerView.SCROLL_STATE_DRAGGING) return;

                    // Record only the first time the user scrolls the carousel after it was shown.
                    SuggestionsMetrics.recordContextualSuggestionsCarouselScrolled();
                    recyclerView.removeOnScrollListener(this);
                }
            });

            ViewGroup.LayoutParams params =
                    new RecyclerView.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
            recyclerView.setLayoutParams(params);

            // Make the recycler view scroll horizontally.
            LinearLayoutManager layoutManager = new LinearLayoutManager(recyclerView.getContext());
            layoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
            recyclerView.setLayoutManager(layoutManager);
            recyclerView.setClipToPadding(false);

            float spaceBetweenCards = recyclerView.getContext().getResources().getDimension(
                    R.dimen.contextual_carousel_space_between_cards);
            int recyclerViewMarginEnd = (int) Math.floor(spaceBetweenCards);
            ApiCompatibilityUtils.setPaddingRelative(recyclerView, 0, 0, recyclerViewMarginEnd, 0);
        }

        @Override
        public void recycle() {
            ((RecyclerView) itemView).setAdapter(null);
            super.recycle();
        }
    }
}