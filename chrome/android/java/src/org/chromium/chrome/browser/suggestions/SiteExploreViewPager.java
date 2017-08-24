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

        // Get the height of the tab layout. ViewPager decors (like TabLayout) and ViewPager pages
        // are both ViewPager children. The decors come first. That's why TabLayout is at index 0
        // and the first page will be at index 1.
        View tabLayout = getChildAt(0);
        tabLayout.measure(
                widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
        int tabLayoutHeight = tabLayout.getMeasuredHeight();

        // Measure what height its first page wants to be. We assume that all pages will have the
        // same height.
        View pageView = getChildAt(1);
        final int pageHeight;
        if (pageView != null) {
            pageView.measure(
                    widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
            pageHeight = pageView.getMeasuredHeight();
        } else {
            pageHeight = 0;
        }

        // Take into account vertical padding.
        int verticalPadding = getPaddingTop() + getPaddingBottom();

        // Set the height of the ViewPager to the sum of the height of the TabLayout, the height
        // of one of its pages and any vertical padding.
        heightMeasureSpec = MeasureSpec.makeMeasureSpec(
                pageHeight + tabLayoutHeight + verticalPadding, MeasureSpec.EXACTLY);

        // Super has to be called again so the new specs are treated as exact measurements.
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
