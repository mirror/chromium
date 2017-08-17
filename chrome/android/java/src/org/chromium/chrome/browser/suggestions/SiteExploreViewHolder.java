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
    private SiteExploreViewHolder.ExploreSectionsAdapter mAdapter;

    public SiteExploreViewHolder(ViewGroup view, int maxTileColumns) {
        super(view);
        mMaxTileColumns = maxTileColumns;
        mSiteExploreViewPager = itemView.findViewById(R.id.site_explore_pager);

        mAdapter = new ExploreSectionsAdapter(itemView.getContext());
        mSiteExploreViewPager.setAdapter(mAdapter);
    }

    @Override
    public void refreshData() {
        mAdapter.updateTiles(mTileGroup.getTileSections());
    }

    @Override
    protected TileView findTileView(SiteSuggestion data) {
        return mAdapter.findTileView(data);
    }

    @Override
    public void recycle() {
        super.recycle();
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
        public String getPageTitle(int position) {
            @TileSectionType
            int sectionType = (mLatestTileSections.keyAt(position));
            int stringRes;
            switch (sectionType) {
                case TileSectionType.PERSONALIZED:
                    stringRes = R.string.ntp_sites_exploration_category_personalized_title;
                    break;
                case TileSectionType.SOCIAL:
                    stringRes = R.string.ntp_sites_exploration_category_social_title;
                    break;
                case TileSectionType.ENTERTAINMENT:
                    stringRes = R.string.ntp_sites_exploration_category_entertainment_title;
                    break;
                case TileSectionType.NEWS:
                    stringRes = R.string.ntp_sites_exploration_category_news_title;
                    break;
                case TileSectionType.ECOMMERCE:
                    stringRes = R.string.ntp_sites_exploration_category_ecommerce_title;
                    break;
                case TileSectionType.TOOLS:
                    stringRes = R.string.ntp_sites_exploration_category_tools_title;
                    break;
                case TileSectionType.TRAVEL:
                    stringRes = R.string.ntp_sites_exploration_category_travel_title;
                    break;
                case TileSectionType.UNKNOWN:
                default:
                    stringRes = R.string.ntp_sites_exploration_category_other_title;
                    break;
            }

            return mContext.getResources().getString(stringRes);
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
