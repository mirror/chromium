// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.view.ViewGroup;

import java.util.List;

/**
 * A {@link SiteSectionViewHolder} specialised in displaying sites as a simple grid of tiles,
 * through
 * {@link TileGridLayout}.
 */
public class TileGridViewHolder extends SiteSectionViewHolder {
    private final TileGridLayout mSectionView;

    private TileGroup mTileGroup;
    private TileRenderer mTileRenderer;

    public TileGridViewHolder(ViewGroup view, int maxRows, int maxColumns) {
        super(view);

        mSectionView = (TileGridLayout) itemView;
        mSectionView.setMaxRows(maxRows);
        mSectionView.setMaxColumns(maxColumns);
    }

    @Override
    public void bindDataSource(TileGroup tileGroup, TileRenderer tileRenderer) {
        mTileGroup = tileGroup;
        mTileRenderer = tileRenderer;
    }

    @Override
    public void refreshData() {
        List<Tile> tiles = mTileGroup.getTileSections().get(TileSectionType.PERSONALIZED);
        assert tiles != null;

        mTileRenderer.renderTileSection(tiles, mSectionView, mTileGroup.getTileSetupDelegate());
        mTileGroup.notifyTilesRendered();
    }

    /**
     * Sets a new icon on the child view with a matching site.
     * @param tile The tile that holds the data to populate the tile view.
     */
    @Override
    public void updateIconView(Tile tile) {
        TileView tileView = mSectionView.getTileView(tile.getData());
        if (tileView != null) tileView.renderIcon(tile);
    }

    /**
     * Updates the visibility of the offline badge on the child view with a matching site.
     * @param tile The tile that holds the data to populate the tile view.
     */
    @Override
    public void updateOfflineBadge(Tile tile) {
        TileView tileView = mSectionView.getTileView(tile.getData());
        if (tileView != null) tileView.renderOfflineBadge(tile);
    }
}
