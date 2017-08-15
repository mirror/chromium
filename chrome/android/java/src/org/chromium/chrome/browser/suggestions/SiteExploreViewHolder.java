// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;

/**
 * A view holder for the Explore UI.
 */

public class SiteExploreViewHolder extends SiteSectionViewHolder {
    private final int mMaxTileColumns;
    private ViewPager mSiteExploreViewPager;
    private TileGroup mTileGroup;
    private TileRenderer mTileRenderer;
    private SiteExploreViewHolder.ExploreSectionsAdapter mAdapter;
    private int mLastSelectedPage;

    public SiteExploreViewHolder(ViewGroup view, int maxTileColumns) {
        super(view);
        mMaxTileColumns = maxTileColumns;
        mLastSelectedPage = 0;
    }

    @Override
    public void bindDataSource(TileGroup tileGroup, TileRenderer tileRenderer) {
        mTileGroup = tileGroup;
        mTileRenderer = tileRenderer;
        mSiteExploreViewPager = itemView.findViewById(R.id.site_explore_pager);

        mAdapter = new ExploreSectionsAdapter(itemView.getContext());
        mSiteExploreViewPager.setAdapter(mAdapter);

        // This is needed to keep the tab that the user selected last, while maybe scrolling away
        // from it.
        mSiteExploreViewPager.clearOnPageChangeListeners();
        mSiteExploreViewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int i, float v, int i1) {}

            @Override
            public void onPageSelected(int i) {
                mLastSelectedPage = i;
            }

            @Override
            public void onPageScrollStateChanged(int i) {}
        });
        mSiteExploreViewPager.setCurrentItem(mLastSelectedPage, false);
    }

    @Override
    public void refreshData() {
        mAdapter.notifyDataSetChanged();
    }

    @Override
    public void updateIconView(Tile tile) {
        TileView tileView = mAdapter.findTileView(tile.getData());
        if (tileView != null) tileView.renderIcon(tile);
    }

    @Override
    public void updateOfflineBadge(Tile tile) {}

    /**
     * Adapter for the Explore UI view holder.
     */
    public class ExploreSectionsAdapter extends PagerAdapter {
        private final Context mContext;

        public ExploreSectionsAdapter(Context context) {
            mContext = context;
        }

        @Override
        public View instantiateItem(ViewGroup container, int position) {
            TileGroup.TileSectionList tileSectionList =
                    mTileGroup.getTileSections().valueAt(position);
            if (tileSectionList == null) return null;

            TileGridLayout layout = tileSectionList.getTileGridLayout();
            if (layout == null) {
                layout = new TileGridLayout(mContext, null);
                layout.setMaxRows(1);
                layout.setMaxColumns(mMaxTileColumns);
                tileSectionList.setTileGridLayout(layout);
            }

            mTileRenderer.renderTileSection(
                    tileSectionList, layout, mTileGroup.getTileSetupDelegate());
            mTileGroup.notifyTilesRendered();

            if (layout.getParent() != null) ((ViewGroup) layout.getParent()).removeView(layout);
            container.addView(layout);
            return layout;
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {}

        @Override
        public int getCount() {
            return mTileGroup.getTileSections().size();
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return mTileGroup.getTileSections().valueAt(position).getSectionTitle();
        }

        @Override
        public boolean isViewFromObject(View view, Object object) {
            return view == object;
        }

        @Nullable
        public TileView findTileView(SiteSuggestion suggestion) {
            TileGridLayout layout =
                    mTileGroup.getTileSections().get(suggestion.sectionType).getTileGridLayout();
            if (layout == null) return null;
            return layout.getTileView(suggestion);
        }
    }
}
