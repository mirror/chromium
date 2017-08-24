// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.View;

/**
 * View Pager for the Site Explore UI. This is needed in order to correctly calculate the height
 * of this section on {@link #onMeasure(int, int)}.
 */
public class SiteExploreViewPager extends ViewPager {
    public SiteExploreViewPager(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // We need to override this method because ViewPager does not properly handle wrap_content
        // as its height measurement. It relies on the container view to give it a height and
        // based on that value will crop its children. So, we will need to measure how much space
        // the ViewPager will need to show its contents.
        // We know that this SiteExploreViewPager will hold a TabLayout and a TileGridLayout at all
        // times, so we can calculate what height it will need to be.

        // We need to call this to let the ViewPager initialize its children pages first. This will
        // call ViewPagerAdapter.initializeItem(), which will add the pages. However, this
        // does not measure the heights of the children - it sets their height depending on its
        // own height, which is 0. That's why we need to measure the children's heights
        // explicitly after that.
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        int neededHeight = 0;

        // Get the required height from the view pager's children. ViewPager decors (e.g. TabLayout)
        // and ViewPager pages are both ViewPager children. The decors come first. We add up the
        // heights of all decors and the height of the first page to get the required height for the
        // view pager. We assume that all pages have the same height.
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            child.measure(
                    widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));

            neededHeight += child.getMeasuredHeight();

            // Break the loop when we see the first page (non decor view).
            if (!((ViewPager.LayoutParams) child.getLayoutParams()).isDecor) {
                break;
            }
        }

        // Take into account vertical padding.
        neededHeight += getPaddingTop() + getPaddingBottom();

        // Set the height of the ViewPager to the sum of the height of the TabLayout, the height
        // of one of its pages and any vertical padding.
        heightMeasureSpec = MeasureSpec.makeMeasureSpec(neededHeight, MeasureSpec.EXACTLY);

        // TODO(galinap): Restrict view pager's width to tile_grid_layout_max_width in NTP.

        // Super method has to be called again so the new specs are treated as exact measurements.
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
