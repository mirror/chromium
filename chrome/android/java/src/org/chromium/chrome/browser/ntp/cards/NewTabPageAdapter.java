// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.Adapter;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.Callback;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ntp.NewTabPageView.NewTabPageManager;
import org.chromium.chrome.browser.ntp.UiConfig;
import org.chromium.chrome.browser.ntp.snippets.SectionHeaderViewHolder;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticleViewHolder;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;

/**
 * A class that handles merging above the fold elements and below the fold cards into an adapter
 * that will be used to back the NTP RecyclerView. The first element in the adapter should always be
 * the above-the-fold view (containing the logo, search box, and most visited tiles) and subsequent
 * elements will be the cards shown to the user
 */
public class NewTabPageAdapter extends Adapter<NewTabPageViewHolder> implements NodeParent {
    private final NewTabPageManager mNewTabPageManager;
    private final View mAboveTheFoldView;
    private final UiConfig mUiConfig;
    private NewTabPageRecyclerView mRecyclerView;

    private final InnerNode mRoot;

    private final AboveTheFoldItem mAboveTheFold = new AboveTheFoldItem();
    private final SectionList mSections;
    private final SignInPromo mSigninPromo;
    private final AllDismissedItem mAllDismissed;
    private final Footer mFooter;
    private final SpacingItem mBottomSpacer = new SpacingItem();

    /**
     * Creates the adapter that will manage all the cards to display on the NTP.
     *
     * @param manager the NewTabPageManager to use to interact with the rest of the system.
     * @param aboveTheFoldView the layout encapsulating all the above-the-fold elements
     *                         (logo, search box, most visited tiles)
     * @param uiConfig the NTP UI configuration, to be passed to created views.
     * @param offlinePageBridge the OfflinePageBridge used to determine if articles are available
     *                              offline.
     *
     */
    public NewTabPageAdapter(NewTabPageManager manager, View aboveTheFoldView, UiConfig uiConfig,
            OfflinePageBridge offlinePageBridge) {
        mNewTabPageManager = manager;
        mAboveTheFoldView = aboveTheFoldView;
        mUiConfig = uiConfig;
        mRoot = new InnerNode();

        mSections = new SectionList(mNewTabPageManager, offlinePageBridge);
        mSigninPromo = new SignInPromo(mNewTabPageManager);
        mAllDismissed = new AllDismissedItem();
        mFooter = new Footer();

        mRoot.addChildren(
                mAboveTheFold, mSections, mSigninPromo, mAllDismissed, mFooter, mBottomSpacer);

        updateAllDismissedVisibility();
        mRoot.setParent(this);
    }

    @Override
    @ItemViewType
    public int getItemViewType(int position) {
        return mRoot.getItemViewType(position);
    }

    @Override
    public NewTabPageViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        assert parent == mRecyclerView;

        switch (viewType) {
            case ItemViewType.ABOVE_THE_FOLD:
                return new NewTabPageViewHolder(mAboveTheFoldView);

            case ItemViewType.HEADER:
                return new SectionHeaderViewHolder(mRecyclerView, mUiConfig);

            case ItemViewType.SNIPPET:
                return new SnippetArticleViewHolder(mRecyclerView, mNewTabPageManager, mUiConfig);

            case ItemViewType.SPACING:
                return new NewTabPageViewHolder(SpacingItem.createView(parent));

            case ItemViewType.STATUS:
                return new StatusCardViewHolder(mRecyclerView, mNewTabPageManager, mUiConfig);

            case ItemViewType.PROGRESS:
                return new ProgressViewHolder(mRecyclerView);

            case ItemViewType.ACTION:
                return new ActionItem.ViewHolder(mRecyclerView, mNewTabPageManager, mUiConfig);

            case ItemViewType.PROMO:
                return new SignInPromo.ViewHolder(mRecyclerView, mNewTabPageManager, mUiConfig);

            case ItemViewType.FOOTER:
                return new Footer.ViewHolder(mRecyclerView, mNewTabPageManager);

            case ItemViewType.ALL_DISMISSED:
                return new AllDismissedItem.ViewHolder(mRecyclerView, mSections);
        }

        assert false : viewType;
        return null;
    }

    @Override
    public void onBindViewHolder(NewTabPageViewHolder holder, final int position) {
        mRoot.onBindViewHolder(holder, position);
    }

    @Override
    public int getItemCount() {
        return mRoot.getItemCount();
    }

    public int getAboveTheFoldPosition() {
        return getChildPositionOffset(mAboveTheFold);
    }

    public int getFirstHeaderPosition() {
        return getFirstPositionForType(ItemViewType.HEADER);
    }

    public int getFirstCardPosition() {
        for (int i = 0; i < getItemCount(); ++i) {
            if (CardViewHolder.isCard(getItemViewType(i))) return i;
        }
        return RecyclerView.NO_POSITION;
    }

    int getLastContentItemPosition() {
        return getChildPositionOffset(hasAllBeenDismissed() ? mAllDismissed : mFooter);
    }

    int getBottomSpacerPosition() {
        return getChildPositionOffset(mBottomSpacer);
    }

    private void updateAllDismissedVisibility() {
        boolean showAllDismissed = hasAllBeenDismissed();
        mAllDismissed.setVisible(showAllDismissed);
        mFooter.setVisible(!showAllDismissed);
    }

    @Override
    public void onItemRangeChanged(TreeNode child, int itemPosition, int itemCount) {
        assert child == mRoot;
        notifyItemRangeChanged(itemPosition, itemCount);
    }

    @Override
    public void onItemRangeInserted(TreeNode child, int itemPosition, int itemCount) {
        assert child == mRoot;
        notifyItemRangeInserted(itemPosition, itemCount);
        mBottomSpacer.refresh();

        updateAllDismissedVisibility();
    }

    @Override
    public void onItemRangeRemoved(TreeNode child, int itemPosition, int itemCount) {
        assert child == mRoot;
        notifyItemRangeRemoved(itemPosition, itemCount);
        mBottomSpacer.refresh();

        updateAllDismissedVisibility();
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        super.onAttachedToRecyclerView(recyclerView);

        // We are assuming for now that the adapter is used with a single RecyclerView.
        // Getting the reference as we are doing here is going to be broken if that changes.
        assert mRecyclerView == null;

        // FindBugs chokes on the cast below when not checked, raising BC_UNCONFIRMED_CAST
        assert recyclerView instanceof NewTabPageRecyclerView;

        mRecyclerView = (NewTabPageRecyclerView) recyclerView;
    }

    /**
     * Dismisses the item at the provided adapter position. Can also cause the dismissal of other
     * items or even entire sections.
     */
    public void dismissItem(int position, Callback<String> itemRemovedCallback) {
        mRoot.dismissItem(position, itemRemovedCallback);
    }

    /**
     * Returns another view holder that should be dismissed at the same time as the provided one.
     */
    public NewTabPageViewHolder getDismissSibling(ViewHolder viewHolder) {
        int swipePos = viewHolder.getAdapterPosition();
        int siblingPosDelta = mRoot.getDismissSiblingPosDelta(swipePos);
        if (siblingPosDelta == 0) return null;

        return (NewTabPageViewHolder) mRecyclerView.findViewHolderForAdapterPosition(
                siblingPosDelta + swipePos);
    }

    private boolean hasAllBeenDismissed() {
        return mSections.isEmpty() && !mSigninPromo.isVisible();
    }

    private int getChildPositionOffset(TreeNode child) {
        return mRoot.getStartingOffsetForChild(child);
    }

    @VisibleForTesting
    SnippetArticle getSuggestionAt(int position) {
        return mRoot.getSuggestionAt(position);
    }

    @VisibleForTesting
    int getFirstPositionForType(@ItemViewType int viewType) {
        int count = getItemCount();
        for (int i = 0; i < count; i++) {
            if (getItemViewType(i) == viewType) return i;
        }
        return RecyclerView.NO_POSITION;
    }

    SectionList getSectionListForTesting() {
        return mSections;
    }

    InnerNode getRootForTesting() {
        return mRoot;
    }
}
