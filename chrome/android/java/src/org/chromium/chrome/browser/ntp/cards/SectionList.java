// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import org.chromium.base.Log;
import org.chromium.chrome.browser.ntp.NewTabPageView.NewTabPageManager;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus;
import org.chromium.chrome.browser.ntp.snippets.CategoryStatus.CategoryStatusEnum;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.ntp.snippets.SnippetsConfig;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * A node in the tree containing a list of all suggestions sections. It listens to changes in the
 * suggestions source and updates the corresponding sections.
 */
public class SectionList extends InnerNode implements SuggestionsSource.Observer {
    private static final String TAG = "Ntp";

    /** Maps suggestion categories to sections, with stable iteration ordering. */
    private final Map<Integer, SuggestionsSection> mSections = new LinkedHashMap<>();
    private final List<TreeNode> mChildren = new ArrayList<>();
    private final NewTabPageManager mNewTabPageManager;
    private final OfflinePageBridge mOfflinePageBridge;

    public SectionList(NodeParent parent, NewTabPageManager newTabPageManager,
            OfflinePageBridge offlinePageBridge) {
        super(parent);
        mNewTabPageManager = newTabPageManager;
        mNewTabPageManager.getSuggestionsSource().setObserver(this);
        mOfflinePageBridge = offlinePageBridge;
    }

    @Override
    public void init() {
        super.init();
        resetSections(/* alwaysAllowEmptySections = */ false);
    }

    @Override
    protected List<TreeNode> getChildren() {
        return mChildren;
    }

    /**
     * Resets the sections, reloading the whole new tab page content.
     * @param alwaysAllowEmptySections Whether sections are always allowed to be displayed when
     *     they are empty, even when they are normally not.
     */
    public void resetSections(boolean alwaysAllowEmptySections) {
        mSections.clear();
        mChildren.clear();

        SuggestionsSource suggestionsSource = mNewTabPageManager.getSuggestionsSource();
        int[] categories = suggestionsSource.getCategories();
        int[] suggestionsPerCategory = new int[categories.length];
        int i = 0;
        for (int category : categories) {
            int categoryStatus = suggestionsSource.getCategoryStatus(category);
            if (categoryStatus == CategoryStatus.LOADING_ERROR
                    || categoryStatus == CategoryStatus.NOT_PROVIDED
                    || categoryStatus == CategoryStatus.CATEGORY_EXPLICITLY_DISABLED)
                continue;

            suggestionsPerCategory[i++] =
                    resetSection(category, categoryStatus, alwaysAllowEmptySections);
        }

        mNewTabPageManager.trackSnippetsPageImpression(categories, suggestionsPerCategory);
    }

    /**
     * Resets the section for {@code category}. Removes the section if there are no suggestions for
     * it and it is not allowed to be empty. Otherwise, creates the section if it is not present
     * yet. Sets the available suggestions on the section.
     * @param category The category for which the section must be reset.
     * @param categoryStatus The category status.
     * @param alwaysAllowEmptySections Whether sections are always allowed to be displayed when
     *     they are empty, even when they are normally not.
     * @return The number of suggestions for the section.
     */
    private int resetSection(@CategoryInt int category, @CategoryStatusEnum int categoryStatus,
            boolean alwaysAllowEmptySections) {
        SuggestionsSource suggestionsSource = mNewTabPageManager.getSuggestionsSource();
        List<SnippetArticle> suggestions = suggestionsSource.getSuggestionsForCategory(category);
        SuggestionsCategoryInfo info = suggestionsSource.getCategoryInfo(category);

        SuggestionsSection section = mSections.get(category);

        // Do not show an empty section if not allowed.
        if (suggestions.isEmpty() && !info.showIfEmpty() && !alwaysAllowEmptySections) {
            if (section != null) removeSection(section);
            return 0;
        }

        // Create the section if needed.
        if (section == null) {
            section = new SuggestionsSection(this, mNewTabPageManager, mOfflinePageBridge, info);
            mSections.put(category, section);
            mChildren.add(section);
            didAddChild(section);
        }

        // Add the new suggestions.
        setSuggestions(category, suggestions, categoryStatus);

        return suggestions.size();
    }

    @Override
    public void onNewSuggestions(@CategoryInt int category) {
        @CategoryStatusEnum
        int status = mNewTabPageManager.getSuggestionsSource().getCategoryStatus(category);

        if (!canLoadSuggestions(category, status)) return;

        // We never want to refresh the suggestions if we already have some content.
        if (mSections.get(category).hasSuggestions()) return;

        List<SnippetArticle> suggestions =
                mNewTabPageManager.getSuggestionsSource().getSuggestionsForCategory(category);

        Log.d(TAG, "Received %d new suggestions for category %d.", suggestions.size(), category);

        // At first, there might be no suggestions available, we wait until they have been fetched.
        if (suggestions.isEmpty()) return;

        setSuggestions(category, suggestions, status);
    }

    @Override
    public void onMoreSuggestions(@CategoryInt int category, List<SnippetArticle> suggestions) {
        @CategoryStatusEnum
        int status = mNewTabPageManager.getSuggestionsSource().getCategoryStatus(category);
        if (!canLoadSuggestions(category, status)) return;

        setSuggestions(category, suggestions, status);
    }

    @Override
    public void onCategoryStatusChanged(@CategoryInt int category, @CategoryStatusEnum int status) {
        // Observers should not be registered for this state.
        assert status != CategoryStatus.ALL_SUGGESTIONS_EXPLICITLY_DISABLED;

        // If there is no section for this category there is nothing to do.
        if (!mSections.containsKey(category)) return;

        switch (status) {
            case CategoryStatus.NOT_PROVIDED:
                // The section provider has gone away. Keep open UIs as they are.
                return;

            case CategoryStatus.CATEGORY_EXPLICITLY_DISABLED:
            case CategoryStatus.LOADING_ERROR:
                // Need to remove the entire section from the UI immediately.
                removeSection(mSections.get(category));
                return;

            case CategoryStatus.SIGNED_OUT:
                resetSection(category, status, /* alwaysAllowEmptySections = */ false);
                return;

            default:
                mSections.get(category).setStatus(status);
                return;
        }
    }

    @Override
    public void onSuggestionInvalidated(@CategoryInt int category, String idWithinCategory) {
        if (!mSections.containsKey(category)) return;
        mSections.get(category).removeSuggestionById(idWithinCategory);
    }

    @Override
    public void onFullRefreshRequired() {
        resetSections(/* alwaysAllowEmptySections = */false);
    }

    private void setSuggestions(@CategoryInt int category, List<SnippetArticle> suggestions,
            @CategoryStatusEnum int status) {
        // Count the number of suggestions before this category.
        int globalPositionOffset = 0;
        for (Map.Entry<Integer, SuggestionsSection> entry : mSections.entrySet()) {
            if (entry.getKey() == category) break;
            globalPositionOffset += entry.getValue().getSuggestionsCount();
        }
        // Assign global indices to the new suggestions.
        for (SnippetArticle suggestion : suggestions) {
            suggestion.mGlobalPosition = globalPositionOffset + suggestion.mPosition;
        }

        mSections.get(category).addSuggestions(suggestions, status);
    }

    private boolean canLoadSuggestions(@CategoryInt int category, @CategoryStatusEnum int status) {
        // We never want to add suggestions from unknown categories.
        if (!mSections.containsKey(category)) return false;

        // The status may have changed while the suggestions were loading, perhaps they should not
        // be displayed any more.
        if (!SnippetsBridge.isCategoryEnabled(status)) {
            Log.w(TAG, "Received suggestions for a disabled category (id=%d, status=%d)", category,
                    status);
            return false;
        }

        return true;
    }

    /**
     * Dismisses a section.
     * @param section The section to be dismissed.
     */
    public void dismissSection(SuggestionsSection section) {
        assert SnippetsConfig.isSectionDismissalEnabled();

        mNewTabPageManager.getSuggestionsSource().dismissCategory(section.getCategory());
        removeSection(section);
    }

    private void removeSection(SuggestionsSection section) {
        mSections.remove(section.getCategory());
        willRemoveChild(section);
        mChildren.remove(section);
    }

    /**
     * Restores any sections that have been dismissed and triggers a new fetch.
     */
    public void restoreDismissedSections() {
        mNewTabPageManager.getSuggestionsSource().restoreDismissedCategories();
        resetSections(/* allowEmptySections = */ true);
        mNewTabPageManager.getSuggestionsSource().fetchRemoteSuggestions();
    }

    /**
     * @return Whether the list of sections is empty.
     */
    public boolean isEmpty() {
        return mSections.isEmpty();
    }

    SuggestionsSection getSectionForTesting(@CategoryInt int categoryId) {
        return mSections.get(categoryId);
    }
}
