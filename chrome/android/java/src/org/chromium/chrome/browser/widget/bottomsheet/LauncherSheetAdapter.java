// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.content.res.Resources;
import android.support.annotation.Nullable;
import android.support.v7.widget.GridLayout;
import android.support.v7.widget.RecyclerView;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.suggestions.SiteSectionViewHolder;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegate;
import org.chromium.chrome.browser.suggestions.Tile;
import org.chromium.chrome.browser.suggestions.TileGroup;
import org.chromium.chrome.browser.suggestions.TileRenderer;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.List;

/**
 * Created by galinap on 9/1/17.
 */

class LauncherSheetAdapter
        extends RecyclerView.Adapter<LauncherSectionViewHolder> implements TileGroup.Observer {
    private static final int MAX_TILE_COLUMNS = 4;
    private final SuggestionsUiDelegate mUiDelegate;
    private final UiConfig mUiConfig;
    private final ContextMenuManager mContextMenuManager;
    private final TileRenderer mTileRenderer;
    private final TileGroup mTileGroup;
    private final RecyclerView mRecyclerView;

    public LauncherSheetAdapter(SuggestionsUiDelegate uiDelegate, UiConfig uiConfig,
            OfflinePageBridge offlinePageBridge, ContextMenuManager contextMenuManager,
            @Nullable TileGroup.Delegate tileGroupDelegate, RecyclerView recyclerView) {
        mUiDelegate = uiDelegate;
        mContextMenuManager = contextMenuManager;

        mUiConfig = uiConfig;

        mTileRenderer = new TileRenderer(ContextUtils.getApplicationContext(),
                SuggestionsConfig.getTileStyle(uiConfig), getTileTitleLines(),
                uiDelegate.getImageFetcher());
        mTileGroup = new TileGroup(mTileRenderer, uiDelegate, contextMenuManager, tileGroupDelegate,
                /* observer = */ this, offlinePageBridge);
        mTileGroup.startObserving(getMaxTileRows() * MAX_TILE_COLUMNS);
        mRecyclerView = recyclerView;
    }

    @Override
    public LauncherSectionViewHolder onCreateViewHolder(ViewGroup viewGroup, int i) {
        return new LauncherSectionViewHolder(viewGroup);
    }

    @Override
    public void onBindViewHolder(LauncherSectionViewHolder launcherSectionViewHolder, int index) {
        List<Tile> tiles = mTileGroup.getTileSections().valueAt(index);
        assert tiles != null;

        GridLayout gridLayout = launcherSectionViewHolder.itemView.findViewById(R.id.grid_layout);
        gridLayout.setColumnCount(calculateColumnCount(mRecyclerView));

        mTileRenderer.renderTileSection(tiles, gridLayout, mTileGroup.getTileSetupDelegate());
        mTileGroup.notifyTilesRendered();
    }

    private int calculateColumnCount(ViewGroup tileContainer) {
        Resources resources = tileContainer.getResources();
        int tileViewWidth = resources.getDimensionPixelSize(R.dimen.tile_view_width);
        return (int) Math.floor((float) tileContainer.getMeasuredWidth() / tileViewWidth);
    }

    @Override
    public void onBindViewHolder(
            LauncherSectionViewHolder holder, int position, List<Object> payloads) {
        if (payloads.isEmpty()) {
            onBindViewHolder(holder, position);
            return;
        }

        for (Object payload : payloads) {
            ((LauncherSectionViewHolder.PartialBindCallback) payload).onResult(holder);
        }
    }

    @Override
    public int getItemCount() {
        return mTileGroup.getTileSections().size();
    }

    public int getMaxTileRows() {
        return 3;
    }

    @Override
    public void onTileDataChanged() {
        notifyDataSetChanged();
    }

    @Override
    public void onTileCountChanged() {
        notifyDataSetChanged();
    }

    @Override
    public void onTileIconChanged(Tile tile) {
        notifyItemChanged(mTileGroup.getTileSections().indexOfKey(tile.getData().sectionType),
                new SiteSectionViewHolder.UpdateIconViewCallback(tile));
    }

    @Override
    public void onTileOfflineBadgeVisibilityChanged(Tile tile) {
        notifyDataSetChanged();
    }

    public int getTileTitleLines() {
        return 1;
    }
}
