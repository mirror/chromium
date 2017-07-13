// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeVisitor;
import org.chromium.chrome.browser.ntp.cards.OptionalLeaf;

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
    public SuggestionsCarousel() {
        setVisible(true);
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        assert holder instanceof ViewHolder;
        ((ViewHolder) holder).onBindViewHolder();
    }

    @Override
    protected int getItemViewType() {
        return ItemViewType.CAROUSEL;
    }

    @Override
    protected void visitOptionalItem(NodeVisitor visitor) {}

    /**
     * View holder for the {@link SuggestionsCarousel}.
     */
    public static class ViewHolder extends NewTabPageViewHolder {
        private SuggestionsCarouselAdapter mCarouselAdapter;
        private SuggestionsUiDelegate mUiDelegate;

        public ViewHolder(RecyclerView parentView, SuggestionsUiDelegate uiDelegate) {
            super(LayoutInflater.from(parentView.getContext())
                            .inflate(R.layout.suggestions_carousel_recycler_view, parentView,
                                    false));

            mUiDelegate = uiDelegate;

            RecyclerView recyclerView = itemView.findViewById(R.id.carousel_recycler_view);

            // Make the recycler view scroll horizontally.
            LinearLayoutManager layoutManager = new LinearLayoutManager(parentView.getContext());
            layoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
            recyclerView.setLayoutManager(layoutManager);

            mCarouselAdapter = new SuggestionsCarouselAdapter(mUiDelegate, recyclerView);
            recyclerView.setAdapter(mCarouselAdapter);
        }

        /* Used to propagate onBindViewHolder() call to all items in the carousel. */
        public void onBindViewHolder() {
            mCarouselAdapter.notifyDataSetChanged();
        }
    }
}