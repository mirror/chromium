// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnScrollListener;
import android.support.v7.widget.Toolbar.OnMenuItemClickListener;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.preferences.privacy.ClearBrowsingDataPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.chrome.browser.widget.selection.SelectableListLayout;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.List;

/**
 * Displays and manages the UI for browsing history.
 */
public class HistoryManager implements OnMenuItemClickListener {
    private static final int FAVICON_MAX_CACHE_SIZE_BYTES = 10 * 1024 * 1024; // 10MB
    private static final int MEGABYTES_TO_BYTES =  1024 * 1024;

    private final Activity mActivity;
    private final SelectableListLayout mSelectableListLayout;
    private final HistoryAdapter mHistoryAdapter;
    private final SelectionDelegate<HistoryItem> mSelectionDelegate;
    private LargeIconBridge mLargeIconBridge;

    /**
     * Creates a new HistoryManager.
     * @param activity The Activity associated with the HistoryManager.
     */
    public HistoryManager(Activity activity) {
        mActivity = activity;

        mSelectionDelegate = new SelectionDelegate<>();
        mHistoryAdapter = new HistoryAdapter(mSelectionDelegate, this);

        mSelectableListLayout =
                (SelectableListLayout) LayoutInflater.from(activity).inflate(
                        R.layout.history_main, null);
        RecyclerView recyclerView = mSelectableListLayout.initializeRecyclerView(mHistoryAdapter);
        mSelectableListLayout.initializeToolbar(R.layout.history_toolbar, mSelectionDelegate,
                R.string.menu_history, null, R.id.normal_menu_group,
                R.id.selection_mode_menu_group, this);
        mSelectableListLayout.initializeEmptyView(
                TintedDrawable.constructTintedDrawable(mActivity.getResources(),
                        R.drawable.history_large),
                R.string.history_manager_empty);

        mHistoryAdapter.initialize();

        recyclerView.addOnScrollListener(new OnScrollListener() {
            @Override
            public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
                if (!mHistoryAdapter.canLoadMoreItems()) return;

                // Load more items if the scroll position is close to the bottom of the list.
                LinearLayoutManager layoutManager =
                        (LinearLayoutManager) recyclerView.getLayoutManager();
                if (layoutManager.findLastVisibleItemPosition()
                        > (mHistoryAdapter.getItemCount() - 25)) {
                    mHistoryAdapter.loadMoreItems();
                }
            }});

        mLargeIconBridge = new LargeIconBridge(Profile.getLastUsedProfile().getOriginalProfile());
        ActivityManager activityManager = ((ActivityManager) ContextUtils
                .getApplicationContext().getSystemService(Context.ACTIVITY_SERVICE));
        int maxSize = Math.min((activityManager.getMemoryClass() / 4) * MEGABYTES_TO_BYTES,
                FAVICON_MAX_CACHE_SIZE_BYTES);
        mLargeIconBridge.createCache(maxSize);
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        if (item.getItemId() == R.id.close_menu_id && !DeviceFormFactor.isTablet(mActivity)) {
            mActivity.finish();
            return true;
        } else if (item.getItemId() == R.id.selection_mode_open_in_new_tab) {
            openItemsInNewTabs(mSelectionDelegate.getSelectedItems(), false);
            mSelectionDelegate.clearSelection();
            return true;
        } else if (item.getItemId() == R.id.selection_mode_open_in_incognito) {
            openItemsInNewTabs(mSelectionDelegate.getSelectedItems(), true);
            mSelectionDelegate.clearSelection();
            return true;
        } else if (item.getItemId() == R.id.selection_mode_delete_menu_id) {
            for (HistoryItem historyItem : mSelectionDelegate.getSelectedItems()) {
                mHistoryAdapter.markItemForRemoval(historyItem);
            }
            mHistoryAdapter.removeItems();
            mSelectionDelegate.clearSelection();
            return true;
        }
        return false;
    }

    /**
     * @return The view that shows the main browsing history UI.
     */
    public ViewGroup getView() {
        return mSelectableListLayout;
    }

    /**
     * Called when the activity/native page is destroyed.
     */
    public void onDestroyed() {
        mSelectableListLayout.onDestroyed();
        mHistoryAdapter.onDestroyed();
        mLargeIconBridge.destroy();
        mLargeIconBridge = null;
    }

    /**
     * Removes the HistoryItem from the history backend and the HistoryAdapter.
     * @param item The HistoryItem to remove.
     */
    public void removeItem(HistoryItem item) {
        if (mSelectionDelegate.isItemSelected(item)) {
            mSelectionDelegate.toggleSelectionForItem(item);
        }
        mHistoryAdapter.markItemForRemoval(item);
        mHistoryAdapter.removeItems();
    }

    /**
     * Open the provided url.
     * @param url The url to open.
     * @param isIncognito Whether to open the url in an incognito tab. If null, the tab
     *                    will open in the current tab model.
     * @param createNewTab Whether a new tab should be created. If false, the item will clobber the
     *                     the current tab.
     */
    public void openUrl(String url, Boolean isIncognito, boolean createNewTab) {
        // Construct basic intent.
        Intent viewIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        viewIntent.putExtra(Browser.EXTRA_APPLICATION_ID,
                mActivity.getApplicationContext().getPackageName());

        // Determine component or class name.
        ComponentName component;
        if (DeviceFormFactor.isTablet(mActivity)) {
            component = mActivity.getComponentName();
        } else {
            component = IntentUtils.safeGetParcelableExtra(
                    mActivity.getIntent(), IntentHandler.EXTRA_PARENT_COMPONENT);
        }
        if (component != null) {
            viewIntent.setComponent(component);
        } else {
            viewIntent.setClass(mActivity, ChromeLauncherActivity.class);
        }

        // Set other intent extras.
        if (isIncognito != null) {
            viewIntent.putExtra(IntentHandler.EXTRA_OPEN_NEW_INCOGNITO_TAB, isIncognito);
        }
        if (createNewTab) viewIntent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);

        // Send intent.
        IntentHandler.startActivityForTrustedIntent(viewIntent, mActivity);
    }

    /**
     * Opens the clear browsing data preference.
     */
    public void openClearBrowsingDataPreference() {
        Intent intent = PreferencesLauncher.createIntentForSettingsPage(mActivity,
                ClearBrowsingDataPreferences.class.getName());
        IntentUtils.safeStartActivity(mActivity, intent);
    }

    /**
     * @return The {@link LargeIconBridge} used to fetch large favicons.
     */
    public LargeIconBridge getLargeIconBridge() {
        return mLargeIconBridge;
    }

    private void openItemsInNewTabs(List<HistoryItem> items, boolean isIncognito) {
        for (HistoryItem item : items) {
            openUrl(item.getUrl(), isIncognito, true);
        }
    }
}