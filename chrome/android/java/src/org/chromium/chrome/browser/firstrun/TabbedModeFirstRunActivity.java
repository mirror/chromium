// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.content.Context;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

/**
 * FirstRunActivity variant that fills the whole screen, but displays the content
 * in a dialog-like layout.
 */
public class TabbedModeFirstRunActivity extends FirstRunActivity {
    @Override
    protected View createContentView() {
        return wrapInDialogLayout(super.createContentView());
    }

    /**
     * Wraps contentView into layout that resembles DialogWhenLarge. The layout centers
     * the content and dims the background to simulate a modal dialog.
     */
    private static View wrapInDialogLayout(View contentView) {
        Context context = contentView.getContext();
        ContentLayout contentLayout = new ContentLayout(context);
        contentLayout.addView(contentView);

        contentLayout.setBackgroundResource(R.drawable.bg_white_dialog);

        // We need an outer layout for two things:
        //   * centering the content
        //   * dimming the background
        FrameLayout outerLayout = new FrameLayout(context);
        outerLayout.addView(contentLayout,
                new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER));
        outerLayout.setBackgroundResource(R.color.modal_dialog_scrim_color);
        return outerLayout;
    }

    /**
     * Layout that sizes itself according to DialogWhenLarge constraints.
     */
    private static class ContentLayout extends FrameLayout {
        TypedValue mFixedWidthMajor = new TypedValue();
        TypedValue mFixedWidthMinor = new TypedValue();
        TypedValue mFixedHeightMajor = new TypedValue();
        TypedValue mFixedHeightMinor = new TypedValue();

        public ContentLayout(Context context) {
            super(context);
            fetchConstraints();
        }

        private void fetchConstraints() {
            // Fetch size constraints. These are copies of corresponding abc_* AppCompat values,
            // because abc_* values are private, and even though corresponding theme attributes
            // are public in AppCompat (e.g. windowFixedWidthMinor), there is no guarantee that
            // AppCompat.DialogWhenLarge is actually defined by AppCompat and not based on
            // system DialogWhenLarge theme.
            // Note that we don't care about the return values, because onMeasure() handles null
            // constraints (and they will be null when the device is not considered "large").
            getContext().getResources().getValue(
                    R.dimen.dialog_fixed_width_minor, mFixedWidthMinor, true);
            getContext().getResources().getValue(
                    R.dimen.dialog_fixed_width_major, mFixedWidthMajor, true);
            getContext().getResources().getValue(
                    R.dimen.dialog_fixed_height_minor, mFixedHeightMinor, true);
            getContext().getResources().getValue(
                    R.dimen.dialog_fixed_height_major, mFixedHeightMajor, true);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            final DisplayMetrics metrics = getContext().getResources().getDisplayMetrics();
            final boolean isPortrait = metrics.widthPixels < metrics.heightPixels;

            final int widthMode = MeasureSpec.getMode(widthMeasureSpec);
            final int heightMode = MeasureSpec.getMode(heightMeasureSpec);

            // If parent doesn't specify our exact size, take constraints into the account.
            // Constraint handling is copied from
            // sdk/sources/android-25/android/support/v7/widget/ContentFrameLayout.java.
            if (widthMode != MeasureSpec.EXACTLY) {
                final TypedValue tvw = isPortrait ? mFixedWidthMinor : mFixedWidthMajor;
                int widthSize = MeasureSpec.getSize(widthMeasureSpec);
                if (tvw != null && tvw.type != TypedValue.TYPE_NULL) {
                    int w = 0;
                    if (tvw.type == TypedValue.TYPE_DIMENSION) {
                        w = (int) tvw.getDimension(metrics);
                    } else if (tvw.type == TypedValue.TYPE_FRACTION) {
                        w = (int) tvw.getFraction(metrics.widthPixels, metrics.widthPixels);
                    }
                    if (w > 0) {
                        // Limit the width if there is a maximum.
                        if (widthMode == MeasureSpec.AT_MOST) {
                            widthSize = Math.min(w, widthSize);
                        } else {
                            widthSize = w;
                        }
                    }
                }
                // This either sets the width calculated from the constraints, or simply
                // takes all of the available space (as if MATCH_PARENT was specified).
                // The behavior is similar to how DialogWhenLarge windows are sized - they
                // are either sized by the constraints, or are full screen, but are never
                // sized based on the content size.
                widthMeasureSpec = MeasureSpec.makeMeasureSpec(widthSize, MeasureSpec.EXACTLY);
            }

            // This is similar to the above block that calculates width.
            if (heightMode != MeasureSpec.EXACTLY) {
                final TypedValue tvh = isPortrait ? mFixedHeightMajor : mFixedHeightMinor;
                int heightSize = MeasureSpec.getSize(heightMeasureSpec);
                if (tvh != null && tvh.type != TypedValue.TYPE_NULL) {
                    int h = 0;
                    if (tvh.type == TypedValue.TYPE_DIMENSION) {
                        h = (int) tvh.getDimension(metrics);
                    } else if (tvh.type == TypedValue.TYPE_FRACTION) {
                        h = (int) tvh.getFraction(metrics.heightPixels, metrics.heightPixels);
                    }
                    if (h > 0) {
                        if (widthMode == MeasureSpec.AT_MOST) {
                            heightSize = Math.min(h, heightSize);
                        } else {
                            heightSize = h;
                        }
                    }
                }
                heightMeasureSpec = MeasureSpec.makeMeasureSpec(heightSize, MeasureSpec.EXACTLY);
            }

            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }
}
