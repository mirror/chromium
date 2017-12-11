// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.textbubble;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.support.annotation.ColorInt;
import android.support.v4.graphics.drawable.DrawableCompat;

import org.chromium.chrome.R;

/**
 * A {@link Drawable} that is a bubble.
 */
public class BubbleDrawable extends Drawable implements Drawable.Callback {
    protected final Rect mCachedBubblePadding = new Rect();
    protected final int mRadiusPx;
    protected final Drawable mBubbleDrawable;

    public BubbleDrawable(Context context) {
        mRadiusPx = context.getResources().getDimensionPixelSize(R.dimen.text_bubble_corner_radius);

        mBubbleDrawable = DrawableCompat.wrap(new ShapeDrawable(
                new RoundRectShape(new float[] {mRadiusPx, mRadiusPx, mRadiusPx, mRadiusPx,
                                           mRadiusPx, mRadiusPx, mRadiusPx, mRadiusPx},
                        null, null)));

        mBubbleDrawable.setCallback(this);
    }

    /**
     * @param color The color to make the bubble and arrow.
     */
    public void setBubbleColor(@ColorInt int color) {
        DrawableCompat.setTint(mBubbleDrawable, color);
        invalidateSelf();
    }

    // Drawable.Callback implementation.
    @Override
    public void invalidateDrawable(Drawable who) {
        invalidateSelf();
    }

    @Override
    public void scheduleDrawable(Drawable who, Runnable what, long when) {
        scheduleSelf(what, when);
    }

    @Override
    public void unscheduleDrawable(Drawable who, Runnable what) {
        unscheduleSelf(what);
    }

    // Drawable implementation.
    @Override
    public void draw(Canvas canvas) {
        mBubbleDrawable.draw(canvas);
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        super.onBoundsChange(bounds);
        if (bounds == null) return;

        // Calculate the bubble bounds.  Account for the arrow size requiring more space.
        mBubbleDrawable.getPadding(mCachedBubblePadding);
        mBubbleDrawable.setBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
    }

    @Override
    public void setAlpha(int alpha) {
        mBubbleDrawable.setAlpha(alpha);
        invalidateSelf();
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        assert false : "Unsupported";
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }

    @Override
    public boolean getPadding(Rect padding) {
        mBubbleDrawable.getPadding(padding);
        return true;
    }
}
