// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.view.View;

import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;

/**
 * The {@code ViewHolder} for the {@link TileGrid}.
 */
public abstract class TileGroupViewHolder extends NewTabPageViewHolder {
    private final TileRenderer.TileGroupLayout mTileGroupLayout;

    private TileGroupViewHolder(View itemView) {
        super(itemView);
        mTileGroupLayout = (TileRenderer.TileGroupLayout) itemView;
    }

    public abstract void resetData(TileGroup tileGroup);

    /**
     * Sets a new icon on the child view with a matching URL.
     * @param tile The tile that holds the data to populate the tile view.
     */
    public void updateIconView(Tile tile) {
        TileView tileView = mTileGroupLayout.getTileView(tile.getData());
        if (tileView != null) tileView.renderIcon(tile);
    }

    /**
     * Updates the visibility of the offline badge on the child view with a matching URL.
     * @param tile The tile that holds the data to populate the tile view.
     */
    public void updateOfflineBadge(Tile tile) {
        TileView tileView = mTileGroupLayout.getTileView(tile.getData());
        if (tileView != null) tileView.renderOfflineBadge(tile);
    }

    public static TileGroupViewHolder create(View view, int maxRows, int maxColumns) {
        if (SuggestionsConfig.useExplore()) {
            assert view instanceof SiteExploreView;
            return new Explore((SiteExploreView) view);
        } else {
            assert view instanceof TileGridLayout;
            return new MostVisited((TileGridLayout) view, maxRows, maxColumns);
        }
    }

    private static class MostVisited extends TileGroupViewHolder {
        private final TileGridLayout mLayout;

        public MostVisited(TileGridLayout layout, int maxRows, int maxColumns) {
            super(layout);
            mLayout = layout;
            mLayout.setMaxRows(maxRows);
            mLayout.setMaxColumns(maxColumns);
        }

        public void resetData(TileGroup tileGroup) {
            tileGroup.renderTiles(mLayout);
        }
    }

    private static class Explore extends TileGroupViewHolder {
        private final SiteExploreView mExploreView;

        public Explore(SiteExploreView view) {
            super(view);
            mExploreView = view;
        }

        public void resetData(TileGroup tileGroup) {
            mExploreView.reset(tileGroup);
        }
    }

    /**
     * Callback to update all the tiles in the view holder.
     */
    public static class UpdateTilesCallback extends PartialBindCallback {
        private final TileGroup mTileGroup;

        public UpdateTilesCallback(TileGroup tileGroup) {
            mTileGroup = tileGroup;
        }

        @Override
        public void onResult(NewTabPageViewHolder holder) {
            assert holder instanceof TileGroupViewHolder;
            ((TileGroupViewHolder) holder).resetData(mTileGroup);
        }
    }

    /**
     * Callback to update the icon view for the view holder.
     */
    public static class UpdateIconViewCallback extends PartialBindCallback {
        private final Tile mTile;

        public UpdateIconViewCallback(Tile tile) {
            mTile = tile;
        }

        @Override
        public void onResult(NewTabPageViewHolder holder) {
            assert holder instanceof TileGroupViewHolder;
            ((TileGroupViewHolder) holder).updateIconView(mTile);
        }
    }

    /**
     * Callback to update the offline badge for the view holder.
     */
    public static class UpdateOfflineBadgeCallback extends PartialBindCallback {
        private final Tile mTile;

        public UpdateOfflineBadgeCallback(Tile tile) {
            mTile = tile;
        }

        @Override
        public void onResult(NewTabPageViewHolder holder) {
            assert holder instanceof TileGroupViewHolder;
            ((TileGroupViewHolder) holder).updateOfflineBadge(mTile);
        }
    }
}
