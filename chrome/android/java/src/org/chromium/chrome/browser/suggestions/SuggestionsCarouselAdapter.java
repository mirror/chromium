// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.ViewGroup;

import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.OfflinePageItem;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.ArrayList;
import java.util.List;

/**
 * Adapter for a horizontal list of suggestions. The individual suggestions are held
 * in {@link ContextualSuggestionsCardViewHolder} instances.
 */
public class SuggestionsCarouselAdapter
        extends RecyclerView.Adapter<ContextualSuggestionsCardViewHolder> {
    private final SuggestionsUiDelegate mUiDelegate;
    private final UiConfig mUiConfig;

    private final ContextMenuManager mContextMenuManager;

    private final List<SnippetArticle> mSuggestionsList;

    public SuggestionsCarouselAdapter(UiConfig uiConfig, SuggestionsUiDelegate uiDelegate,
            ContextMenuManager contextMenuManager, OfflinePageBridge offlinePageBridge) {
        mUiDelegate = uiDelegate;
        mUiConfig = uiConfig;
        mContextMenuManager = contextMenuManager;
        mSuggestionsList = new ArrayList<>();

        OfflineModelObserver mOfflineModelObserver = new OfflineModelObserver(offlinePageBridge);
    }

    @Override
    public ContextualSuggestionsCardViewHolder onCreateViewHolder(ViewGroup viewGroup, int i) {
        return new ContextualSuggestionsCardViewHolder(
                viewGroup, mUiConfig, mUiDelegate, mContextMenuManager);
    }

    @Override
    public void onBindViewHolder(ContextualSuggestionsCardViewHolder holder, int i) {
        holder.onBindViewHolder(mSuggestionsList.get(i));
    }

    @Override
    public void onViewRecycled(ContextualSuggestionsCardViewHolder holder) {
        holder.recycle();
    }

    @Override
    public int getItemCount() {
        return mSuggestionsList.size();
    }

    /**
     * Set the new contextual suggestions to be shown in the suggestions carousel and update the UI.
     *
     * @param suggestions The new suggestions to be shown in the suggestions carousel.
     */
    public void setSuggestions(List<SnippetArticle> suggestions) {
        mSuggestionsList.clear();
        mSuggestionsList.addAll(suggestions);

        notifyDataSetChanged();
    }

    private class OfflineModelObserver extends SuggestionsOfflineModelObserver<SnippetArticle> {
        public OfflineModelObserver(OfflinePageBridge bridge) {
            super(bridge);
        }

        @Override
        public void onSuggestionOfflineIdChanged(SnippetArticle suggestion, OfflinePageItem item) {
            boolean isPrefetched = item != null
                    && TextUtils.equals(item.getClientId().getNamespace(),
                               OfflinePageBridge.CONTEXTUAL_SUGGESTIONS_NAMESPACE);

            Long newId = item == null ? null : item.getOfflineId();

            int index = mSuggestionsList.indexOf(suggestion);
            // The suggestions could have been removed / replaced in the meantime.
            if (index == -1) return;

            Long oldId = suggestion.getOfflinePageOfflineId();
            suggestion.setOfflinePageOfflineId(newId);
            suggestion.setIsPrefetched(isPrefetched);

            if ((oldId == null) == (newId == null)) return;
            notifyDataSetChanged();
        }

        @Override
        public Iterable<SnippetArticle> getOfflinableSuggestions() {
            return mSuggestionsList;
        }
    }
}