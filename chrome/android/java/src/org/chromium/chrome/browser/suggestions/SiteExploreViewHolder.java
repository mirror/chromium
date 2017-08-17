// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v4.view.PagerAdapter;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;

import java.util.List;

/**
 * A view holder for the Explore UI.
 */
public class SiteExploreViewHolder extends SiteSectionViewHolder {
    private final int mMaxTileColumns;
    private SiteExploreViewPager mSiteExploreViewPager;
    private TileGroup mTileGroup;
    private TileRenderer mTileRenderer;
    private SiteExploreViewHolder.ExploreSectionsAdapter mAdapter;

    public SiteExploreViewHolder(ViewGroup view, int maxTileColumns) {
        super(view);
        mMaxTileColumns = maxTileColumns;
        mSiteExploreViewPager = itemView.findViewById(R.id.site_explore_pager);

        mAdapter = new ExploreSectionsAdapter(itemView.getContext());
        mSiteExploreViewPager.setAdapter(mAdapter);
    }

    @Override
    public void bindDataSource(TileGroup tileGroup, TileRenderer tileRenderer) {
        mTileGroup = tileGroup;
        mTileRenderer = tileRenderer;
    }

    @Override
    public void refreshData() {
        // Calling just notifyDataSetChanged() here does not work as the dataset in the ViewPager
        // are the tile sections. These don't change - it's just a tile in one of the sections that
        // is removed.

        mAdapter.updateTiles(mTileGroup.getTileSections());
    }

    @Override
    public void updateIconView(Tile tile) {
        TileView tileView = mAdapter.findTileView(tile.getData());
        if (tileView != null) tileView.renderIcon(tile);
    }

    @Override
    public void updateOfflineBadge(Tile tile) {
        TileView tileView = mAdapter.findTileView(tile.getData());
        if (tileView != null) tileView.renderOfflineBadge(tile);
    }

    @Override
    public void recycle() {
        super.recycle();
        //        mSiteExploreViewPager.setAdapter(null);
    }

    /**
     * Adapter for the Explore UI view holder.
     */
    public class ExploreSectionsAdapter extends PagerAdapter {
        private final Context mContext;
        private SparseArray<TileGridLayout> mTileSectionLayouts;
        private SparseArray<List<Tile>> mLatestTileSections;

        public ExploreSectionsAdapter(Context context) {
            mContext = context;
            mTileSectionLayouts = new SparseArray<TileGridLayout>();
        }

        @Override
        public View instantiateItem(ViewGroup container, int position) {
            List<Tile> tileSectionList = mLatestTileSections.valueAt(position);
            if (tileSectionList == null) return null;

            @TileSectionType
            int sectionType = mTileGroup.getTileSections().keyAt(position);
            TileGridLayout layout = mTileSectionLayouts.get(sectionType);
            if (layout == null) {
                layout = new TileGridLayout(mContext, null);

                layout.setMaxRows(1);
                layout.setMaxColumns(mMaxTileColumns);
                mTileRenderer.renderTileSection(
                        tileSectionList, layout, mTileGroup.getTileSetupDelegate());
                mTileSectionLayouts.put(sectionType, layout);
            }

            if (sectionType == TileSectionType.PERSONALIZED) {
                mTileGroup.notifyTilesRendered();
            }

            container.addView(layout);
            return layout;
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            container.removeView((View) object);
            mTileSectionLayouts.remove(mTileSectionLayouts.keyAt(position));
        }

        @Override
        public int getCount() {
            return mLatestTileSections == null ? 0 : mLatestTileSections.size();
        }

        @Override
        public CharSequence getPageTitle(int position) {
            @TileSectionType
            int sectionType = (mLatestTileSections.keyAt(position));
            switch (sectionType) {
                case TileSectionType.ECOMMERCE:
                    return "Ecommerce";
                case TileSectionType.ENTERTAINMENT:
                    return "Entertainment";
                case TileSectionType.LAST:
                    return "Last";
                case TileSectionType.NEWS:
                    return "News";
                case TileSectionType.PERSONALIZED:
                    return "Personalized";
                case TileSectionType.SOCIAL:
                    return "Social";
                case TileSectionType.TOOLS:
                    return "Tools";
                case TileSectionType.UNKNOWN:
                default:
                    return "Other";
            }
        }

        @Override
        public boolean isViewFromObject(View view, Object object) {
            return view == object;
        }

        @Nullable
        public TileView findTileView(SiteSuggestion suggestion) {
            TileGridLayout layout = mTileSectionLayouts.get(suggestion.sectionType);
            if (layout == null) return null;
            return layout.getTileView(suggestion);
        }

        public void updateTiles(SparseArray<List<Tile>> freshTileSections) {
            mLatestTileSections = freshTileSections;
            for (int i = 0; i < mTileSectionLayouts.size(); i++) {
                @TileSectionType
                int sourceType = mTileSectionLayouts.keyAt(i);
                mTileRenderer.renderTileSection(freshTileSections.get(sourceType),
                        mTileSectionLayouts.get(sourceType), mTileGroup.getTileSetupDelegate());
            }
            notifyDataSetChanged();
        }
    }
}
