// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.graphics.Rect;
import android.support.v7.widget.RecyclerView;
import android.view.View;

/**
 * A class that manages applying padding to various views in the bottom sheet.
 *
 * This is used especially in the case of a transparent bottom navigation menu, since special
 * consideration needs to be taken to offset content by the height of the bottom navigation
 * menu, so it doesn't get occluded.
 */
public class BottomSheetPaddingManager {
    /**
     * A class that handles applying padding to a specific view.
     *
     * @param <T> The specific {@link View} type to be managed by this PaddingHandler.
     */
    public abstract class PaddingHandler<T extends View> {
        // The view managed by this padding handler.
        protected T mView;

        public PaddingHandler(T view) {
            mView = view;
        }

        public abstract void applyPadding(Rect padding);
    }

    /**
     * Returns a PaddingHandler that handles a {@link View}, creating a new one if one doesn't
     * already exist.
     *
     * @param view The {@link View} to retrieve or create a PaddingHandler for.
     * @return A PaddingHandler that manages the this View.
     */
    public PaddingHandler<?> handlerForView(View view) {
        PaddingHandler<? extends View> handler;
        if (view instanceof RecyclerView) {
            handler = new RecyclerViewPaddingHandler((RecyclerView) view);
        } else {
            handler = new GenericPaddingHandler(view);
        }
        return handler;
    }

    /**
     * Applies additional padding to the supplied {@link View}, if an appropriate PaddingHandler
     * can be instantiated for it.
     *
     * @param view The {@link View} we're applying additional padding to.
     * @param padding A {@link Rect} containing the additional padding to be applied.
     */
    public void applyPaddingToView(View view, Rect padding) {
        PaddingHandler<? extends View> handler = handlerForView(view);
        if (handler == null) return;
        handler.applyPadding(padding);
    }

    /**
     * A padding handler for {@link RecyclerView}s, such as the suggestions {@link RecyclerView} for
     * the NTP.
     *
     * Top padding is applied to the top element in the {@link RecyclerView} only, and bottom
     * padding is added only to the bottom element. The idea is to pad the entire list of elements,
     * not each element individually.
     */
    public class RecyclerViewPaddingHandler extends PaddingHandler<RecyclerView> {
        private class PaddingDecorator extends RecyclerView.ItemDecoration {
            private Rect mPadding;

            public PaddingDecorator(Rect padding) {
                mPadding = padding;
            }

            @Override
            public void getItemOffsets(
                    Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                if (mPadding.bottom > 0
                        && (parent.getChildAdapterPosition(view) == state.getItemCount() - 1)) {
                    outRect.bottom = mPadding.bottom;
                }
                if (mPadding.top > 0 && (parent.getChildAdapterPosition(view) == 0)) {
                    outRect.top = mPadding.top;
                }
                outRect.left = mPadding.left;
                outRect.right = mPadding.right;
            }
        }

        public RecyclerViewPaddingHandler(RecyclerView view) {
            super(view);
        }

        @Override
        public void applyPadding(Rect padding) {
            mView.addItemDecoration(new PaddingDecorator(padding));
        }
    }

    /**
     * A generic padding handler.
     */
    public class GenericPaddingHandler extends PaddingHandler<View> {
        public GenericPaddingHandler(View view) {
            super(view);
        }

        @Override
        public void applyPadding(Rect padding) {
            mView.setPadding(mView.getPaddingLeft() + padding.left,
                    mView.getPaddingTop() + padding.top, mView.getPaddingRight() + padding.right,
                    mView.getPaddingBottom() + padding.bottom);
        }
    }
}
