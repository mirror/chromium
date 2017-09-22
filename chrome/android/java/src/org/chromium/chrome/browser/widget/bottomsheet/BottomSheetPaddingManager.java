// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.graphics.Rect;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import java.util.HashMap;
import java.util.Map;

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
        /** The View managed by this PaddingHandler. **/
        protected T mView;

        public PaddingHandler(T view) {
            mView = view;
        }

        /**
         * Returns the padding values for the View managed by this PaddingHandler.
         *
         * @return The Rect containing the padding values of the view handled by this
         * PaddingHandler.
         */
        protected Rect getViewPaddingRect() {
            return new Rect(mView.getPaddingLeft(), mView.getPaddingTop(), mView.getPaddingRight(),
                    mView.getPaddingBottom());
        }

        /**
         * Applies additional padding to the padding that's already set on the View managed by this
         * PaddingHandler
         *
         * @param padding A Rect containing the padding values to be added to the View.
         */
        protected void applyPaddingRect(Rect padding) {
            Rect currentPadding = getViewPaddingRect();
            mView.setPadding(currentPadding.left + padding.left, currentPadding.top + padding.top,
                    currentPadding.right + padding.right, currentPadding.bottom + padding.bottom);
        }

        public abstract void applyPadding(Rect padding);
    }

    private Map<View, PaddingHandler<? extends View>> mPaddingHandlers =
            new HashMap<View, PaddingHandler<? extends View>>();

    /**
     * Returns a PaddingHandler that handles a {@link View}, creating a new one if one doesn't
     * already exist.
     *
     * @param view The {@link View} to retrieve or create a PaddingHandler for.
     * @return A PaddingHandler that manages the this View.
     */
    public PaddingHandler<?> handlerForView(View view) {
        if (view == null) return null;
        if (mPaddingHandlers.containsKey(view)) {
            return mPaddingHandlers.get(view);
        }
        return createAndStore(view);
    }

    public void applyPaddingToView(View view, Rect padding) {
        PaddingHandler<? extends View> handler = handlerForView(view);
        if (handler == null) return;
        handler.applyPadding(padding);
    }

    /**
     * Creates a new PaddingHandler for the {@link View} sent in.
     *
     * @param view The {@link View} to create a new PaddingHandler for.
     * @return A new PaddingHandler.
     */
    private PaddingHandler<? extends View> createAndStore(View view) {
        assert !mPaddingHandlers.containsKey(view);
        PaddingHandler<? extends View> handler;
        if (view instanceof RecyclerView) {
            handler = new RecyclerViewPaddingHandler((RecyclerView) view);
        } else {
            handler = new GenericPaddingHandler(view);
        }
        mPaddingHandlers.put(view, handler);
        return handler;
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
        // Whether or not the padding has already been applied, so we avoid doing it multiple times.
        private boolean mAppliedPadding;

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
            if (mAppliedPadding) return;
            mView.addItemDecoration(new PaddingDecorator(padding));
            mAppliedPadding = true;
        }
    }

    /**
     * A generic padding handler.
     */
    public class GenericPaddingHandler extends PaddingHandler<View> {
        // Whether or not the padding has already been applied, so we avoid doing it multiple times.
        private boolean mPaddingApplied;

        public GenericPaddingHandler(View view) {
            super(view);
        }

        @Override
        public void applyPadding(Rect padding) {
            if (mPaddingApplied) return;
            applyPaddingRect(padding);
            mPaddingApplied = true;
        }
    }
}
