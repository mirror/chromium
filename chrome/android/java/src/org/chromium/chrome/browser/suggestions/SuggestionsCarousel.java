// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeVisitor;
import org.chromium.chrome.browser.ntp.cards.OptionalLeaf;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.widget.displaystyle.DisplayStyleObserver;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.List;

/**
 * This is an optional item in Chrome Home in the bottom sheet. The carousel is a {@link
 * RecyclerView} which is used to display contextual suggestions to the user according to the
 * website they are currently looking at. The carousel uses a {@link SuggestionsCarouselAdapter} to
 * fetch contextual suggestions from the native side and construct {@link
 * ContextualSuggestionsCardViewHolder}s which will populate the recycler view.
 *
 * When there is no context, i.e. the user is on a native page, the carousel will not be shown.
 */
public class SuggestionsCarousel extends OptionalLeaf {
    public static final int SPACE_BETWEEN_CARDS = 20;
    private final SuggestionsCarouselAdapter mAdapter;

    public SuggestionsCarousel(UiConfig uiConfig, SuggestionsUiDelegate uiDelegate) {
        mAdapter = new SuggestionsCarouselAdapter(uiConfig, uiDelegate, this);
        uiConfig.addObserver(new DisplayStyleObserver() {
            @Override
            public void onDisplayStyleChanged(UiConfig.DisplayStyle newDisplayStyle) {
                mAdapter.notifyDataSetChanged();
            }
        });
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        mAdapter.notifyDataSetChanged();
    }

    @Override
    protected int getItemViewType() {
        return ItemViewType.CAROUSEL;
    }

    @Override
    protected void visitOptionalItem(NodeVisitor visitor) {}

    public void newContextualSuggestionsAvailable(List<SnippetArticle> newSuggestions) {
        setVisibilityInternal(newSuggestions != null && !newSuggestions.isEmpty());
        mAdapter.notifyNewSuggestionsAvailable(newSuggestions);
    }

    public View createView(ViewGroup parentView) {
        RecyclerView recyclerView = new RecyclerView(parentView.getContext());

        ViewGroup.LayoutParams params = new RecyclerView.LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        recyclerView.setLayoutParams(params);

        // Make the recycler view scroll horizontally.
        LinearLayoutManager layoutManager = new LinearLayoutManager(parentView.getContext());
        layoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
        layoutManager.offsetChildrenHorizontal(SPACE_BETWEEN_CARDS);
        recyclerView.setLayoutManager(layoutManager);
        recyclerView.setClipToPadding(false);
        ApiCompatibilityUtils.setPaddingRelative(
                recyclerView, SPACE_BETWEEN_CARDS, 0, SPACE_BETWEEN_CARDS, 0);

        recyclerView.setAdapter(mAdapter);

        return recyclerView;
    }
}