// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

public class ThemedWindowFrame {
    public static View wrapContentView(int themeId, View contentView) {
        Context context = contentView.getContext();

        ContentLayout contentLayout = new ContentLayout(context, themeId);
        contentLayout.addView(contentView);

        FrameLayout outerLayout = new FrameLayout(context);
        outerLayout.addView(contentLayout,
                new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER));
        return outerLayout;
    }

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

        private void setBackground(int themeId) {
            TypedArray a = getContext().obtainStyledAttributes(
                    themeId, new int[] {android.R.attr.windowBackground});

            int resourceId = a.getResourceId(0, 0);
            if (resourceId != 0) {
                setBackgroundResource(resourceId);
            }

            a.recycle();
        }

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
