// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

import java.util.List;

/**
 * Layout that presents {@link SiteSuggestion}s as tiles that can be browsed
 */
public class SiteExploreView extends FrameLayout implements TileRenderer.TileGroupLayout {
    private Adapter mAdapter;

    public SiteExploreView(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public void reset(Adapter adapter) {
        mAdapter = adapter;
        this.<ViewPager>findViewById(R.id.site_explore_pager).setAdapter(adapter);
    }

    @Override
    @Nullable
    public TileView getTileView(SiteSuggestion suggestion) {
        TileGridLayout layout = mAdapter.mSections.get(suggestion.sectionType).mContainer;
        if (layout == null) return null;
        return layout.getTileView(suggestion);
    }

    // TODO(dgn): move out
    private static class SiteSection {
        @TileSectionType
        public final int sectionType;
        public final List<Tile> tiles;

        @Nullable
        private TileGridLayout mContainer;

        public SiteSection(int sectionType, List<Tile> tiles) {
            this.sectionType = sectionType;
            this.tiles = tiles;
        }
    }

    /** Adapter responsible of managing the data for the Sites exploration view. */
    // TODO(dgn): move out
    public static class Adapter extends PagerAdapter {
        private final Context mContext;
        private TileRenderer mTileRenderer;

        private final int mPageRowCount = 1;
        private final int mPageColumnCount = 4;

        private SparseArray<SiteSection> mSections = new SparseArray<>();
        private TileGroup mTileGroup;

        public Adapter(Context context, TileRenderer tileRenderer, TileGroup tileGroup) {
            mContext = context;

            mTileRenderer = tileRenderer;
            mTileGroup = tileGroup;

            reloadData();
        }

        public void reloadData() {
            mSections.clear();
            SparseArray<List<Tile>> section = mTileGroup.getTileSections();
            for (int i = 0; i < section.size(); i++) {
                int sectionType = section.keyAt(i);
                mSections.put(sectionType, new SiteSection(sectionType, section.valueAt(i)));
            }
            notifyDataSetChanged();
        }

        @Override
        public Object instantiateItem(ViewGroup container, int position) {
            TileGridLayout layout = new TileGridLayout(mContext);
            layout.setMaxRows(mPageRowCount);
            layout.setMaxColumns(mPageColumnCount);

            SiteSection section = mSections.valueAt(position);
            mTileRenderer.renderTileSection(
                    section.tiles, layout, mTileGroup.getTileSetupDelegate());
            section.mContainer = layout;

            container.addView(layout);
            return section;
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            assert object == mSections.valueAt(position);
            mSections.valueAt(position).mContainer = null;
        }

        @Override
        public int getCount() {
            return mSections.size();
        }

        @Override
        public boolean isViewFromObject(View view, Object o) {
            return view == ((SiteSection) o).mContainer;
        }

        @Override
        public CharSequence getPageTitle(int position) {
            switch (mSections.valueAt(position).sectionType) {
                case TileSectionType.PERSONALIZED:
                    return "Personalized";
                case TileSectionType.TOOLS:
                    return "Tools";
                default:
                    return "Unknown";
            }
        }
    }
}
