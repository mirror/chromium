// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.v7.widget.RecyclerView;
import android.view.ViewGroup;

import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.List;

/**
 * Adapter for a {@link RecyclerView} which is used to show contextual suggestions in Chrome Home
 * in the bottom sheet. It populates the recycler view representing the {@link SuggestionsCarousel}
 * with cards from the {@link ContextualSuggestionsCardViewHolder}.
 *
 * It fetches information about all suggestions from {@link SuggestionsSource}.
 */
public class SuggestionsCarouselAdapter
        extends RecyclerView.Adapter<ContextualSuggestionsCardViewHolder> {
    private final SuggestionsUiDelegate mUiDelegate;
    private final SuggestionsCarousel mCarousel;
    private final UiConfig mUiConfig;
    private List<SnippetArticle> mSuggestions;

    public SuggestionsCarouselAdapter(
            UiConfig uiConfig, SuggestionsUiDelegate uiDelegate, SuggestionsCarousel carousel) {
        mUiDelegate = uiDelegate;
        mUiConfig = uiConfig;
        mCarousel = carousel;
    }

    @Override
    public ContextualSuggestionsCardViewHolder onCreateViewHolder(ViewGroup viewGroup, int i) {
        return new ContextualSuggestionsCardViewHolder(viewGroup, mUiConfig, mUiDelegate);
    }

    @Override
    public void onBindViewHolder(final ContextualSuggestionsCardViewHolder holder, final int i) {
        if (mSuggestions == null) return;

        if (mSuggestions.isEmpty()) return;

        holder.onBindViewHolder(mSuggestions.get(i));
    }

    @Override
    public int getItemCount() {
        return mSuggestions == null ? 0 : mSuggestions.size();
    }

    public void notifyNewSuggestionsAvailable(List<SnippetArticle> suggestions) {
        mSuggestions = suggestions;
        notifyDataSetChanged();
    }
}