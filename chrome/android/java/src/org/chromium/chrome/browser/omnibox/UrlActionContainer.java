// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.base.ApiCompatibilityUtils;

/**
 * A container for the action buttons (delete, mic) next to the omnibox.
 */
public class UrlActionContainer extends FrameLayout {
    public UrlActionContainer(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = 0;
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            if (child.getVisibility() == GONE) continue;
            width += getViewWidth(child);
        }
        super.onMeasure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY), heightMeasureSpec);
    }

    @Override
    public void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (getChildCount() < 2) return;

        int margin = 0;
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            if (child.getVisibility() == GONE) continue;

            MarginLayoutParams layoutParams = (MarginLayoutParams) child.getLayoutParams();
            ApiCompatibilityUtils.setMarginStart(layoutParams, margin);
            child.setLayoutParams(layoutParams);
            margin += Math.max(layoutParams.width, child.getWidth());
        }
    }

    private int getViewWidth(View v) {
        ViewGroup.LayoutParams layoutParams = v.getLayoutParams();
        return Math.max(layoutParams.width, v.getWidth());
    }
}