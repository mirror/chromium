// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

import android.content.res.Resources;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.LinearLayout;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.ContextMenuManager.TouchEnabledDelegate;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.snackbar.SnackbarManager;
import org.chromium.chrome.browser.suggestions.DestructionObserver;
import org.chromium.chrome.browser.suggestions.SuggestionsDependencyFactory;
import org.chromium.chrome.browser.suggestions.SuggestionsMetrics;
import org.chromium.chrome.browser.suggestions.SuggestionsNavigationDelegate;
import org.chromium.chrome.browser.suggestions.SuggestionsNavigationDelegateImpl;
import org.chromium.chrome.browser.suggestions.SuggestionsSheetVisibilityChangeObserver;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegateImpl;
import org.chromium.chrome.browser.suggestions.TileGroupDelegateImpl;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * Created by galinap on 9/1/17.
 */

class LauncherBottomSheetContent implements BottomSheet.BottomSheetContent {
    private final TileGroupDelegateImpl mTileGroupDelegate;
    private final BottomSheet mSheet;
    private final SuggestionsUiDelegateImpl mSuggestionsUiDelegate;
    private final ContextMenuManager mContextMenuManager;
    private final RecyclerView mRecyclerView;
    private final SuggestionsSheetVisibilityChangeObserver mBottomSheetObserver;

    public LauncherBottomSheetContent(final ChromeActivity activity, final BottomSheet sheet,
            TabModelSelector tabModelSelector, SnackbarManager snackbarManager) {
        mSheet = sheet;
        SuggestionsDependencyFactory depsFactory = SuggestionsDependencyFactory.getInstance();
        Profile profile = Profile.getLastUsedProfile();
        SuggestionsNavigationDelegate navigationDelegate =
                new SuggestionsNavigationDelegateImpl(activity, profile, sheet, tabModelSelector);
        mRecyclerView = new RecyclerView(sheet.getContext());
        mRecyclerView.setLayoutParams(new LinearLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT));
        mRecyclerView.setLayoutManager(new LinearLayoutManager(sheet.getContext()));

        Resources resources = mRecyclerView.getResources();
        int paddingTop = resources.getDimensionPixelOffset(R.dimen.bottom_control_container_height);
        ApiCompatibilityUtils.setPaddingRelative(mRecyclerView, 0, paddingTop, 0, 0);

        mTileGroupDelegate =
                new TileGroupDelegateImpl(activity, profile, navigationDelegate, snackbarManager);
        mSuggestionsUiDelegate = new SuggestionsUiDelegateImpl(
                depsFactory.createSuggestionSource(profile), depsFactory.createEventReporter(),
                navigationDelegate, profile, sheet, activity.getReferencePool());

        TouchEnabledDelegate touchEnabledDelegate = new TouchEnabledDelegate() {
            @Override
            public void setTouchEnabled(boolean enabled) {
                activity.getBottomSheet().setTouchEnabled(enabled);
            }
        };

        mContextMenuManager =
                new ContextMenuManager(activity, navigationDelegate, touchEnabledDelegate);
        activity.getWindowAndroid().addContextMenuCloseListener(mContextMenuManager);
        mSuggestionsUiDelegate.addDestructionObserver(new DestructionObserver() {
            @Override
            public void onDestroy() {
                activity.getWindowAndroid().removeContextMenuCloseListener(mContextMenuManager);
            }
        });

        UiConfig uiConfig = new UiConfig(mRecyclerView);

        final LauncherSheetAdapter adapter = new LauncherSheetAdapter(mSuggestionsUiDelegate,
                uiConfig, OfflinePageBridge.getForProfile(profile), mContextMenuManager,
                mTileGroupDelegate, mRecyclerView);

        mBottomSheetObserver = new SuggestionsSheetVisibilityChangeObserver(this, activity) {
            @Override
            public void onContentShown(boolean isFirstShown) {
                if (isFirstShown) {
                    // adapter.refreshSuggestions();
                    mSuggestionsUiDelegate.getEventReporter().onSurfaceOpened();

                    // Set the adapter on the RecyclerView after updating it, to avoid sending
                    // notifications that might confuse its internal state.
                    // See https://crbug.com/756514.
                    mRecyclerView.setAdapter(adapter);
                    mRecyclerView.scrollToPosition(0);
                }

                // TODO(galinap): what metrics do we need?
                // SuggestionsMetrics.recordSurfaceVisible();
            }

            @Override
            public void onContentHidden() {
                // TODO(galinap): what metrics do we need?
                SuggestionsMetrics.recordSurfaceHidden();
            }

            @Override
            public void onContentStateChanged(@BottomSheet.SheetState int contentState) {}

            @Override
            public void onSheetClosed() {
                super.onSheetClosed();
                mRecyclerView.setAdapter(null);
            }

            @Override
            public void onSheetOffsetChanged(float heightFraction) {}
        };
    }

    @Override
    public View getContentView() {
        return mRecyclerView;
    }

    @Nullable
    @Override
    public View getToolbarView() {
        return null;
    }

    @Override
    public boolean isUsingLightToolbarTheme() {
        return false;
    }

    @Override
    public boolean isIncognitoThemedContent() {
        return false;
    }

    @Override
    public int getVerticalScrollOffset() {
        return mRecyclerView.computeVerticalScrollOffset();
    }

    @Override
    public void destroy() {}

    @Override
    public int getType() {
        return BottomSheetContentController.TYPE_LAUNCHER;
    }

    @Override
    public boolean applyDefaultTopPadding() {
        return false;
    }
}
