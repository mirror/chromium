// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.annotation.CallSuper;
import android.view.View;

import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;

import java.util.List;

/**
 * The {@code ViewHolder} for the {@link TileGrid}.
 */
public abstract class TileGroupViewHolder extends NewTabPageViewHolder {
    private final TileRenderer.TileGroupLayout mTileGroupLayout;
    protected TileGroup mTileGroup;
    protected TileRenderer mTileRenderer;

    private TileGroupViewHolder(View itemView) {
        super(itemView);
        mTileGroupLayout = (TileRenderer.TileGroupLayout) itemView;
    }

    @CallSuper
    public void init(TileGroup tileGroup, TileRenderer tileRenderer) {
        mTileGroup = tileGroup;
        mTileRenderer = tileRenderer;
    }

    public abstract void refreshData();

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
        if (view instanceof SiteExploreView) {
            assert SuggestionsConfig.useExplore();
            // Ignores the specified row and columns dimensions, used internal ones.
            return new Explore((SiteExploreView) view);
        } else {
            assert view instanceof TileGridLayout;
            return new Grid((TileGridLayout) view, maxRows, maxColumns);
        }
    }

    private static class Grid extends TileGroupViewHolder {
        private final TileGridLayout mLayout;

        public Grid(TileGridLayout layout, int maxRows, int maxColumns) {
            super(layout);
            mLayout = layout;
            mLayout.setMaxRows(maxRows);
            mLayout.setMaxColumns(maxColumns);
        }

        @Override
        public void refreshData() {
            assert mTileGroup.getTileSections().size() == 1;
            List<Tile> tiles = mTileGroup.getTileSections().get(TileSectionType.PERSONALIZED);
            assert tiles != null;
            mTileRenderer.renderTileSection(tiles, mLayout, mTileGroup.getTileSetupDelegate());
            mTileGroup.onTilesRendered();
        }
    }

    private static class Explore extends TileGroupViewHolder {
        private final SiteExploreView.Adapter mExploreAdapter;

        public Explore(SiteExploreView view) {
            super(view);
            mExploreAdapter =
                    new SiteExploreView.Adapter(view.getContext(), mTileRenderer, mTileGroup);
        }

        @Override
        public void refreshData() {
            mExploreAdapter.reloadData();
        }
    }

    /**
     * Callback to update all the tiles in the view holder.
     */
    public static class UpdateTilesCallback extends PartialBindCallback {
        @Override
        public void onResult(NewTabPageViewHolder holder) {
            assert holder instanceof TileGroupViewHolder;
            ((TileGroupViewHolder) holder).refreshData();
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
