// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.graphics.Rect;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import java.util.List;

/**
 * A utility class that handles applying padding to various views in the bottom sheet.
 *
 * This is used especially in the case of a transparent bottom navigation menu, since special
 * consideration needs to be taken to offset content by the height of the bottom navigation
 * menu, so it doesn't get occluded.
 */
public class BottomSheetPaddingUtils {
    /**
     * Applies additional padding to the supplied {@link View}, using an appropriate specific
     * function if a more specific type of {@link View} calls for it.
     *
     * @param view The {@link View} we're applying additional padding to.
     * @param padding A {@link Rect} containing the additional padding to be applied.
     */
    public static void applyPaddingToView(View view, Rect padding) {
        if (view instanceof RecyclerView) {
            applyRecyclerViewPadding((RecyclerView) view, padding);
        } else {
            applyGenericViewPadding(view, padding);
        }
    }

    /**
     * Applies additional padding to several {@link View}s using #applyPaddingToView.
     *
     * @param views The {@link List} of {@link View}s we're applying additional padding to.
     * @param padding A {@link Rect} containing the additional padding to be applied.
     */
    public static void applyPaddingToViews(List<View> views, Rect padding) {
        for (View view : views) {
            applyPaddingToView(view, padding);
        }
    }

    /**
     * Applies padding for {@link RecyclerView}s, such as the suggestions {@link RecyclerView} for
     * the NTP.
     *
     * Top padding is applied to the top element in the {@link RecyclerView} only, and bottom
     * padding is added only to the bottom element. The idea is to pad the entire list of elements,
     * not each element individually.
     *
     * @param view The {@link RecyclerView} that needs additional padding.
     * @param padding The additional padding to be applied.
     */
    private static void applyRecyclerViewPadding(RecyclerView view, Rect padding) {
        view.addItemDecoration(new RecyclerView.ItemDecoration() {
            @Override
            public void getItemOffsets(
                    Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                if (padding.bottom > 0
                        && (parent.getChildAdapterPosition(view) == state.getItemCount() - 1)) {
                    outRect.bottom = padding.bottom;
                }
                if (padding.top > 0 && (parent.getChildAdapterPosition(view) == 0)) {
                    outRect.top = padding.top;
                }
                outRect.left = padding.left;
                outRect.right = padding.right;
            }
        });
    }

    /**
     * A generic padding handler to be used as a fallback if there's no handler for a more specific
     * {@link View} type.
     *
     * @param view The {@link View} that needs additional padding.
     * @param padding The additional padding to be applied.
     */
    private static void applyGenericViewPadding(View view, Rect padding) {
        view.setPadding(view.getPaddingLeft() + padding.left, view.getPaddingTop() + padding.top,
                view.getPaddingRight() + padding.right, view.getPaddingBottom() + padding.bottom);
    }
}
