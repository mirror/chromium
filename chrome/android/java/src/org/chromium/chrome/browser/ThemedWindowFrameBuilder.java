// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

/**
 * Builds a frame around content view to simulate window layout with a specific theme.
 *
 * Useful if you want to make dialog activity fill the screen, but still present content view
 * in a dialog-like layout.
 *
 * This class works only with AppCompat themes (ctor throws an exception otherwise).
 *
 * Only a few window-related attributes from the theme are taken into account: layout,
 * background, dim alpha.
 *
 * Result of build() method is meant to be set (or added) as a content view.
 */
public class ThemedWindowFrameBuilder {
    private Context mContext;
    private int mThemeId;
    private boolean mDimBehind;
    private View mContentView;

    /**
     * Creates the builder.
     * @param context context to use
     * @param themeId AppCompat theme to take window layout from
     */
    public ThemedWindowFrameBuilder(Context context, int themeId) {
        ensureAppCompatTheme(context, themeId);
        mContext = context;
        mThemeId = themeId;
    }

    /**
     * Enables dimming (similar to specifying FLAG_DIM_BEHIND on a window).
     */
    public ThemedWindowFrameBuilder setDimBehind() {
        mDimBehind = true;
        return this;
    }

    /**
     * Sets content view.
     */
    public ThemedWindowFrameBuilder setContentView(View contentView) {
        mContentView = contentView;
        return this;
    }

    /**
     * Builds window frame. Content view must be set before calling this method.
     * @return window frame that wraps the content view
     */
    public View build() {
        ContentLayout contentLayout = new ContentLayout(mContext, mThemeId);
        contentLayout.addView(mContentView);

        // We can't return contentLayout as is, because it'll be stretched if set as a content
        // view, ignoring window layout from the theme. We need to wrap contentLayout in other
        // layout and use WRAP_CONTENT constraints. We'll also use that other layout for:
        //   * centering (since dialog windows are centered)
        //   * dimming
        FrameLayout windowFrame = new FrameLayout(mContext);
        windowFrame.setBackground(createDimDrawable());
        windowFrame.addView(contentLayout,
                new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER));
        return windowFrame;
    }

    private Drawable createDimDrawable() {
        if (!mDimBehind) {
            return null;
        }

        TypedArray a = mContext.obtainStyledAttributes(
                mThemeId, new int[] {android.R.attr.backgroundDimAmount});
        float dimAmount = a.getFloat(0 /* index */, 0.5f);
        a.recycle();

        ColorDrawable drawable = new ColorDrawable();
        drawable.setColor(Color.BLACK);
        drawable.setAlpha((int) (Math.max(0f, Math.min(dimAmount, 1f)) * 255));
        return drawable;
    }

    /**
     * Makes sure that the theme is AppCompatTheme, and throws an exception otherwise. The check
     * is copied from sdk/sources/android-25/android/support/v7/app/AppCompatDelegateImplV9.java.
     */
    private static void ensureAppCompatTheme(Context context, int themeId) {
        TypedArray a = context.obtainStyledAttributes(R.styleable.AppCompatTheme);
        boolean hasWindowActionBar = a.hasValue(R.styleable.AppCompatTheme_windowActionBar);
        a.recycle();
        if (!hasWindowActionBar) {
            throw new IllegalStateException(
                    "You need to use a Theme.AppCompat theme (or descendant) with this activity.");
        }
    }

    /**
     * This class simulates layout of a window with a specific theme. Layout code was copied
     * from sdk/sources/android-25/android/support/v7/widget/ContentFrameLayout.java
     */
    private static class ContentLayout extends FrameLayout {
        TypedValue mMinWidthMajor = new TypedValue();
        TypedValue mMinWidthMinor = new TypedValue();
        TypedValue mFixedWidthMajor = new TypedValue();
        TypedValue mFixedWidthMinor = new TypedValue();
        TypedValue mFixedHeightMajor = new TypedValue();
        TypedValue mFixedHeightMinor = new TypedValue();

        public ContentLayout(Context context, int themeId) {
            super(context);
            fetchWindowConstraints(themeId);
            setBackground(themeId);
        }

        /**
         * Sets background drawable as specified by the theme's windowBackground attribute.
         */
        private void setBackground(int themeId) {
            TypedArray a = getContext().obtainStyledAttributes(
                    themeId, new int[] {android.R.attr.windowBackground});

            int resourceId = a.getResourceId(0 /* index */, 0);
            if (resourceId != 0) {
                setBackgroundResource(resourceId);
            }

            a.recycle();
        }

        /**
         * Fetches various layout constraints from the theme.
         */
        private void fetchWindowConstraints(int themeId) {
            TypedArray a = getContext().obtainStyledAttributes(themeId, R.styleable.AppCompatTheme);

            a.getValue(R.styleable.AppCompatTheme_windowMinWidthMajor, mMinWidthMajor);
            a.getValue(R.styleable.AppCompatTheme_windowMinWidthMinor, mMinWidthMinor);

            if (a.hasValue(R.styleable.AppCompatTheme_windowFixedWidthMajor)) {
                a.getValue(R.styleable.AppCompatTheme_windowFixedWidthMajor, mFixedWidthMajor);
            }
            if (a.hasValue(R.styleable.AppCompatTheme_windowFixedWidthMinor)) {
                a.getValue(R.styleable.AppCompatTheme_windowFixedWidthMinor, mFixedWidthMinor);
            }
            if (a.hasValue(R.styleable.AppCompatTheme_windowFixedHeightMajor)) {
                a.getValue(R.styleable.AppCompatTheme_windowFixedHeightMajor, mFixedHeightMajor);
            }
            if (a.hasValue(R.styleable.AppCompatTheme_windowFixedHeightMinor)) {
                a.getValue(R.styleable.AppCompatTheme_windowFixedHeightMinor, mFixedHeightMinor);
            }

            a.recycle();
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            final DisplayMetrics metrics = getContext().getResources().getDisplayMetrics();
            final boolean isPortrait = metrics.widthPixels < metrics.heightPixels;

            final int widthMode = MeasureSpec.getMode(widthMeasureSpec);
            final int heightMode = MeasureSpec.getMode(heightMeasureSpec);

            boolean fixedWidth = false;
            if (widthMode == MeasureSpec.AT_MOST) {
                final TypedValue tvw = isPortrait ? mFixedWidthMinor : mFixedWidthMajor;
                if (tvw != null && tvw.type != TypedValue.TYPE_NULL) {
                    int w = 0;
                    if (tvw.type == TypedValue.TYPE_DIMENSION) {
                        w = (int) tvw.getDimension(metrics);
                    } else if (tvw.type == TypedValue.TYPE_FRACTION) {
                        w = (int) tvw.getFraction(metrics.widthPixels, metrics.widthPixels);
                    }
                    if (w > 0) {
                        final int widthSize = MeasureSpec.getSize(widthMeasureSpec);
                        widthMeasureSpec = MeasureSpec.makeMeasureSpec(
                                Math.min(w, widthSize), MeasureSpec.EXACTLY);
                        fixedWidth = true;
                    }
                }
            }

            if (heightMode == MeasureSpec.AT_MOST) {
                final TypedValue tvh = isPortrait ? mFixedHeightMajor : mFixedHeightMinor;
                if (tvh != null && tvh.type != TypedValue.TYPE_NULL) {
                    int h = 0;
                    if (tvh.type == TypedValue.TYPE_DIMENSION) {
                        h = (int) tvh.getDimension(metrics);
                    } else if (tvh.type == TypedValue.TYPE_FRACTION) {
                        h = (int) tvh.getFraction(metrics.heightPixels, metrics.heightPixels);
                    }
                    if (h > 0) {
                        final int heightSize = MeasureSpec.getSize(heightMeasureSpec);
                        heightMeasureSpec = MeasureSpec.makeMeasureSpec(
                                Math.min(h, heightSize), MeasureSpec.EXACTLY);
                    }
                }
            }

            super.onMeasure(widthMeasureSpec, heightMeasureSpec);

            int width = getMeasuredWidth();
            boolean measure = false;

            widthMeasureSpec = MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY);

            if (!fixedWidth && widthMode == MeasureSpec.AT_MOST) {
                final TypedValue tv = isPortrait ? mMinWidthMinor : mMinWidthMajor;
                if (tv != null && tv.type != TypedValue.TYPE_NULL) {
                    int min = 0;
                    if (tv.type == TypedValue.TYPE_DIMENSION) {
                        min = (int) tv.getDimension(metrics);
                    } else if (tv.type == TypedValue.TYPE_FRACTION) {
                        min = (int) tv.getFraction(metrics.widthPixels, metrics.widthPixels);
                    }
                    if (width < min) {
                        widthMeasureSpec = MeasureSpec.makeMeasureSpec(min, MeasureSpec.EXACTLY);
                        measure = true;
                    }
                }
            }

            if (measure) {
                super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            }
        }
    }
}
