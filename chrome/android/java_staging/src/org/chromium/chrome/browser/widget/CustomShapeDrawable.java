// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;

import org.chromium.chrome.browser.enhancedbookmarks.EnhancedBookmarkSalientImageView.BaseSalientDrawable;

/**
 * Base class for drawables that are to control what shape they take.
 */
public abstract class CustomShapeDrawable extends Drawable {

    /**
     * Circular drawable that makes a round mask on top of bitmap.
     */
    public static class CircularDrawable extends CustomShapeDrawable {
        /**
         * Create a drawable based on the bitmap of the salient image.
         */
        public CircularDrawable(Bitmap bitmap) {
            super(bitmap);
        }

        /**
         * Create a drawable based on the pure color.
         */
        public CircularDrawable(int color) {
            super(color);
        }

        @Override
        protected void onBoundsChange(Rect bounds) {
            mRect.set(0, 0, bounds.width(), bounds.height());
            super.onBoundsChange(bounds);
        }

        @Override
        public void draw(Canvas canvas) {
            canvas.drawCircle(mRect.centerX(), mRect.centerY(), mRect.height() * 0.5f, mPaint);
        }
    }

    /**
     * A {@link CircularDrawable} with dark background.
     */
    public static class DarkBackgroundCircularDrawable extends CircularDrawable {
        private static final int FOLDER_CIRCLE_BACKGROUND_COLOR = Color.parseColor("#9E9E9E");
        private Paint mBackgroundPaint = new Paint();

        public DarkBackgroundCircularDrawable(Bitmap bitmap) {
            super(bitmap);
            shouldScale(false);
            mBackgroundPaint.setColor(FOLDER_CIRCLE_BACKGROUND_COLOR);
            mBackgroundPaint.setAntiAlias(true);
            mPaint.setColorFilter(new PorterDuffColorFilter(Color.WHITE, PorterDuff.Mode.SRC_IN));
        }

        @Override
        public void draw(Canvas canvas) {
            canvas.drawCircle(mRect.centerX(), mRect.centerY(), mRect.height() * 0.5f,
                    mBackgroundPaint);
            super.draw(canvas);
        }
    }

    /**
     * Drawable that has rounded top corners, with image or pure color as content.
     */
    public static class TopRoundedCornerDrawable extends BaseSalientDrawable {
        private final float mRadius;

        /**
         * Create a drawable based on bitmap of salient image as well as radius.
         */
        public TopRoundedCornerDrawable(Bitmap bitmap, float radius) {
            super(bitmap);
            mRadius = radius;
        }

        /**
         * Create a drawable based on the pure color as well as radius.
         */
        public TopRoundedCornerDrawable(int color, float radius) {
            super(color);
            mRadius = radius;
        }

        @Override
        protected void onBoundsChange(Rect bounds) {
            // We avoid drawing bottom corners just by making it taller by the radius.
            mRect.set(0, 0, bounds.width(), bounds.height() + mRadius);
            super.onBoundsChange(bounds);
        }

        @Override
        public void draw(Canvas canvas) {
            canvas.drawRoundRect(mRect, mRadius, mRadius, mPaint);
        }
    }

    protected RectF mRect = new RectF();
    protected Paint mPaint = new Paint();
    private int mBitmapWidth;
    private int mBitmapHeight;
    private boolean mShouldScale = true;

    @Override
    protected void onBoundsChange(Rect bounds) {
        // Subclasses must override this function. It should first set up correct boundary of
        // mRect and then call super.onBoundsChange().
        if (mBitmapWidth > 0 && mBitmapHeight > 0) {
            BitmapShader shader = (BitmapShader) mPaint.getShader();

            float scale = 1.0f;
            if (mShouldScale) {
                scale = Math.max((float) bounds.width() / mBitmapWidth,
                        (float) bounds.height() / mBitmapHeight);
            }
            float dx = (bounds.width() - mBitmapWidth * scale) * 0.5f + bounds.left;
            float dy = (bounds.height() - mBitmapHeight * scale) * 0.5f + bounds.top;

            Matrix matrix = new Matrix();
            matrix.setScale(scale, scale);
            matrix.postTranslate(dx, dy);
            shader.setLocalMatrix(matrix);
        }
    }

    protected CustomShapeDrawable(Bitmap bitmap) {
        mPaint.setAntiAlias(true);
        mPaint.setShader(
                new BitmapShader(bitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP));
        mBitmapWidth = bitmap.getWidth();
        mBitmapHeight = bitmap.getHeight();
    }

    protected CustomShapeDrawable(int color) {
        mPaint.setAntiAlias(true);
        mPaint.setColor(color);
        mBitmapWidth = 0;
        mBitmapHeight = 0;
    }

    /**
     * @param shouldScale True to let bitmap be scaled to match the size of
     *                    the drawable. False to let it maintain its original size,
     *                    regardless of the size of the drawable.
     */
    public void shouldScale(boolean shouldScale) {
        mShouldScale = shouldScale;
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }

    @Override
    public void setAlpha(int alpha) {
        mPaint.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        mPaint.setColorFilter(cf);
    }

}
