// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.graphics.drawable.Drawable;
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
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * Item that allows the user to perform an action on the NTP.
 */
public class ActionItem extends OptionalLeaf {
    private final SuggestionsCategoryInfo mCategoryInfo;
    private final SuggestionsSection mParentSection;
    private final SuggestionsRanker mSuggestionsRanker;

    private boolean mImpressionTracked;
    private int mPerSectionRank = -1;
    // TODO(peconn): Do we need this?
    private boolean mEnabled;
    private boolean mLoading;

    public ActionItem(SuggestionsSection section, SuggestionsRanker ranker) {
        mCategoryInfo = section.getCategoryInfo();
        mParentSection = section;
        mSuggestionsRanker = ranker;
        mEnabled = true;
        setVisibilityInternal(
                mCategoryInfo.getAdditionalAction() != ContentSuggestionsAdditionalAction.NONE);
    }

    @Override
    public int getItemViewType() {
        return ItemViewType.ACTION;
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        assert holder instanceof ViewHolder;
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
        if (!mEnabled) return;

        uiDelegate.getEventReporter().onMoreButtonClicked(this);

        switch (mCategoryInfo.getAdditionalAction()) {
            // TODO(peconn): Can we just get rid of this case?
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

    /** Used to enable/disable the action of this item. */
    public void setEnabled(boolean enabled) {
        mEnabled = enabled;
    }

    public boolean isLoading() {
        return mLoading;
    }

    /** Switches between the button and the loading spinner. */
    public void setLoading(boolean loading) {
        mLoading = loading;
        notifyItemChanged(0, new NewTabPageViewHolder.PartialBindCallback() {
            @Override
            public void onResult(NewTabPageViewHolder result) {
                assert result instanceof ViewHolder;
                ((ViewHolder) result).update();
            }
        });
    }

    /** ViewHolder associated to {@link ItemViewType#ACTION}. */
    public static class ViewHolder extends CardViewHolder implements ContextMenuManager.Delegate {
        private final ProgressIndicatorView mProgress;
        private final Button mActionButton;
        private final Drawable mCardBackground;

        private ActionItem mActionListItem;

        public ViewHolder(final SuggestionsRecyclerView recyclerView,
                ContextMenuManager contextMenuManager, final SuggestionsUiDelegate uiDelegate,
                UiConfig uiConfig) {
            super(R.layout.new_tab_page_action_card, recyclerView, uiConfig, contextMenuManager);
            mProgress = (ProgressIndicatorView) itemView.findViewById(R.id.snippets_progress);
            mActionButton = (Button) itemView.findViewById(R.id.action_button);

            mCardBackground = itemView.getBackground();

            mActionButton.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            mActionListItem.performAction(uiDelegate);
                        }
                    });

            new ImpressionTracker(itemView, new ImpressionTracker.Listener() {
                @Override
                public void onImpression() {
                    if (mActionListItem != null && !mActionListItem.mImpressionTracked) {
                        mActionListItem.mImpressionTracked = true;
                        uiDelegate.getEventReporter().onMoreButtonShown(mActionListItem);
                    }
                }
            });
        }

        public void onBindViewHolder(ActionItem item) {
            super.onBindViewHolder();
            mActionListItem = item;

            update();
        }

        public void update() {
            if (mActionListItem.isLoading()) {
                // TODO(peconn): Remove some of the redundancy.
                mActionButton.setVisibility(View.GONE);

                mActionListItem.setEnabled(false);

                // mProgress.showDelayed();
                mProgress.show();

                itemView.setBackground(null);
            } else {
                mActionButton.setVisibility(View.VISIBLE);

                mActionListItem.setEnabled(true);

                mProgress.hide();

                itemView.setBackground(mCardBackground);
            }
        }
    }
}
