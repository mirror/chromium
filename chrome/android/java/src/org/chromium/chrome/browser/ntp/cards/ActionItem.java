// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.annotation.LayoutRes;
import android.view.View;
import android.widget.Button;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.suggestions.ContentSuggestionsAdditionalAction;
import org.chromium.chrome.browser.suggestions.SuggestionsMetrics;
import org.chromium.chrome.browser.suggestions.SuggestionsRanker;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * Item that allows the user to perform an action on the NTP.
 */
public class ActionItem extends OptionalLeaf {
    public @interface State {
        int HIDDEN = 0;
        int BUTTON = 1;
        int LOADING = 2;
    }

    private final SuggestionsCategoryInfo mCategoryInfo;
    private final SuggestionsSection mParentSection;
    private final SuggestionsRanker mSuggestionsRanker;

    private boolean mImpressionTracked;
    private int mPerSectionRank = -1;
    private @State int mState;

    public ActionItem(SuggestionsSection section, SuggestionsRanker ranker) {
        mCategoryInfo = section.getCategoryInfo();
        mParentSection = section;
        mSuggestionsRanker = ranker;
        updateState(State.BUTTON);
    }

    @Override
    public int getItemViewType() {
        return ItemViewType.ACTION;
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        mSuggestionsRanker.rankActionItem(this, mParentSection);
        ((ViewHolder) holder).onBindViewHolder(this);
    }

    @Override
    public void visitOptionalItem(NodeVisitor visitor) {
        visitor.visitActionItem(mCategoryInfo.getAdditionalAction());
    }

    @CategoryInt
    public int getCategory() {
        return mCategoryInfo.getCategory();
    }

    public void setPerSectionRank(int perSectionRank) {
        mPerSectionRank = perSectionRank;
    }

    public int getPerSectionRank() {
        return mPerSectionRank;
    }

    @VisibleForTesting
    void performAction(SuggestionsUiDelegate uiDelegate) {
        assert mState == State.BUTTON;

        uiDelegate.getEventReporter().onMoreButtonClicked(this);

        switch (mCategoryInfo.getAdditionalAction()) {
            case ContentSuggestionsAdditionalAction.VIEW_ALL:
                SuggestionsMetrics.recordActionViewAll();
                mCategoryInfo.performViewAllAction(uiDelegate.getNavigationDelegate());
                return;
            case ContentSuggestionsAdditionalAction.FETCH:
                if (mParentSection.getSuggestionsCount() == 0
                        && mParentSection.getCategoryInfo().isRemote()) {
                    uiDelegate.getSuggestionsSource().fetchRemoteSuggestions();
                } else {
                    mParentSection.fetchSuggestions();
                }
                return;
            case ContentSuggestionsAdditionalAction.NONE:
            default:
                // Should never be reached.
                assert false;
        }
    }

    public void updateState(@State int requestedState) {
        setVisibilityInternal(requestedState != State.HIDDEN
                && mCategoryInfo.getAdditionalAction() != ContentSuggestionsAdditionalAction.NONE);
        @State
        int newState = isVisible() ? requestedState : State.HIDDEN;
        if (mState == newState) return;

        mState = newState;
        if (mState != State.HIDDEN) {
            notifyItemChanged(0, new UpdateStateCallback(newState));
        }
    }

    public @State int getState() {
        return mState;
    }

    /** ViewHolder associated to {@link ItemViewType#ACTION}. */
    public static class ViewHolder extends CardViewHolder implements ContextMenuManager.Delegate {
        private ActionItem mActionListItem;
        private final ProgressIndicatorView mProgressIndicator;
        private final Button mButton;

        public ViewHolder(final SuggestionsRecyclerView recyclerView,
                ContextMenuManager contextMenuManager, final SuggestionsUiDelegate uiDelegate,
                UiConfig uiConfig) {
            super(getLayout(), recyclerView, uiConfig, contextMenuManager);

            mProgressIndicator = itemView.findViewById(R.id.progress_indicator);
            mButton = itemView.findViewById(R.id.action_button);
            mButton.setOnClickListener(v -> mActionListItem.performAction(uiDelegate));

            new ImpressionTracker(itemView, () -> {
                if (mActionListItem != null && !mActionListItem.mImpressionTracked) {
                    mActionListItem.mImpressionTracked = true;
                    uiDelegate.getEventReporter().onMoreButtonShown(mActionListItem);
                }
            });
        }

        public void onBindViewHolder(ActionItem item) {
            super.onBindViewHolder();
            mActionListItem = item;
            setState(item.mState);
        }

        @LayoutRes
        private static int getLayout() {
            return FeatureUtilities.isChromeHomeEnabled()
                    ? R.layout.content_suggestions_action_card_modern
                    : R.layout.new_tab_page_action_card;
        }

        private void setState(@State int state) {
            assert state != State.HIDDEN;

            if (state == State.BUTTON) {
                mButton.setVisibility(View.VISIBLE);
                mProgressIndicator.hide();
            } else if (state == State.LOADING) {
                mButton.setVisibility(View.GONE);
                mProgressIndicator.show();
            } else {
                // Not even HIDDEN is supported as the item should not be able to receive updates.
                assert false : "ActionViewHolder got notified of an unsupported state";
            }
        }
    }

    private static class UpdateStateCallback extends NewTabPageViewHolder.PartialBindCallback {
        private final @State int mState;

        public UpdateStateCallback(@State int state) {
            mState = state;
        }

        @Override
        public void onResult(NewTabPageViewHolder viewHolder) {
            ((ViewHolder) viewHolder).setState(mState);
        }
    }
}
