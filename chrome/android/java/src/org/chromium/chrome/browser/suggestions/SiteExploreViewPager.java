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
        // as its height measurement. This is because different pages might have different heights
        // and the ViewPager doesn't know which one to choose.
        // We know that this SiteExploreViewPager will hold a TabLayout and a TileGridLayout at all
        // times, so we can calculate its height explicitly.

        // Need to call this to let the ViewPager initialize its children pages.
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        // Get the height of the tab layout.
        View tabLayout = getChildAt(0);
        tabLayout.measure(
                widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
        int tabLayoutHeight = tabLayout.getMeasuredHeight();

        // Get the height of the first of its pages. We assume that all pages will have the same
        // height.
        View pageView = getChildAt(1);
        int pageHeight = 0;
        if (pageView != null) {
            pageView.measure(
                    widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
            pageHeight = pageView.getMeasuredHeight();
        }

        // Set the height of the ViewPager to the sum of the height of the TabLayout and the height
        // of one of its pages.
        heightMeasureSpec =
                MeasureSpec.makeMeasureSpec(pageHeight + tabLayoutHeight, MeasureSpec.EXACTLY);

        // Need to call this again to get the true height measurements.
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
