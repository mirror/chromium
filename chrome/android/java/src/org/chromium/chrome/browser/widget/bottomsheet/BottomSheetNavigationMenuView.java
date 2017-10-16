// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.chrome.browser.widget.bottomsheet.base.BottomNavigationItemView;
import org.chromium.chrome.browser.widget.bottomsheet.base.BottomNavigationMenuView;

/**
 * An implementation of {@link BottomNavigationMenuView} specifically for the Chrome Home bottom
 * navigation menu.
 */
public class BottomSheetNavigationMenuView extends BottomNavigationMenuView {
    private int mMenuWidth;

    public BottomSheetNavigationMenuView(Context context) {
        super(context);
    }

    public BottomSheetNavigationMenuView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected BottomNavigationItemView getNewItemViewInstance() {
        return new BottomSheetNavigationItemView(getContext());
    }

    /**
     * Spaces menu items apart based on the minimum width of the bottom navigation menu, so the
     * items are equally spaced in landscape and portrait mode.
     *
     * @param layoutWidth Width of the navigation menu's container.
     * @param layoutHeight Height of the navigation menu's container.
     */
    public void updateMenuItemSpacingForMinWidth(int layoutWidth, int layoutHeight) {
        if (mButtons.length == 0) return;

        int menuWidth = Math.min(layoutWidth, layoutHeight);
        if (menuWidth != mMenuWidth) {
            mMenuWidth = menuWidth;
            int itemWidth = menuWidth / mButtons.length;
            for (int i = 0; i < mButtons.length; i++) {
                BottomNavigationItemView item = mButtons[i];

                LayoutParams layoutParams = (LayoutParams) item.getLayoutParams();
                layoutParams.width = itemWidth;
                item.setLayoutParams(layoutParams);
            }
        }
    }
}