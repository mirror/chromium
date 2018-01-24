// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;

import org.chromium.base.ApiCompatibilityUtils;

/**
 * Provides a {@Rect} for the location of a {@View} in its window. Allows observers to be
 * notified of changes to the View's position through {@link ViewRectProviderObserver}.
 */
public class ViewRectProvider implements ViewTreeObserver.OnGlobalLayoutListener,
                                         View.OnAttachStateChangeListener,
                                         ViewTreeObserver.OnPreDrawListener {
    /**
     * An observer to be notified of changes to the {@Rect} position for the associated
     * {@link View}.
     */
    public interface ViewRectProviderObserver {
        /** Called when the {@link Rect} location for the {@link View} has changed. */
        void onRectChanged(Rect newRect);

        /** Called when the {@link Rect} for the {@link View} is no longer visible. */
        void onRectHidden();
    }

    private final int[] mCachedWindowCoordinates = new int[2];
    private final Rect mRect = new Rect();
    private final Rect mInsetRect = new Rect();
    private final View mView;

    /** If not {@code null}, the {@link ViewTreeObserver} that we are registered to. */
    private ViewTreeObserver mViewTreeObserver;

    private ViewRectProviderObserver mObserver;

    /**
     * Creates an instance of a {@link ViewRectProvider}.
     * @param view The {@link View} used to generate a {@link Rect}.
     */
    public ViewRectProvider(View view) {
        mView = view;
    }

    /**
     * Specifies the inset values in pixels that determine how to shrink the {@link View} bounds
     * when creating the anchor {@link Rect}.
     */
    public void setInsetPx(int left, int top, int right, int bottom) {
        mInsetRect.set(left, top, right, bottom);
        if (mObserver != null) refreshAnchorBounds();
    }

    /**
     * Start observing changes to the {@link View}'s position in the window.
     * @param observer The {@link ViewRectProviderObserver} to be notified of changes.
     */
    public void startObserving(ViewRectProviderObserver observer) {
        mObserver = observer;
        mView.addOnAttachStateChangeListener(this);
        mViewTreeObserver = mView.getViewTreeObserver();
        mViewTreeObserver.addOnGlobalLayoutListener(this);
        mViewTreeObserver.addOnPreDrawListener(this);

        refreshAnchorBounds();
    }

    /**
     * Stop observing changes to the {@link View}'s position in the window.
     */
    public void stopObserving() {
        mView.removeOnAttachStateChangeListener(this);

        if (mViewTreeObserver != null && mViewTreeObserver.isAlive()) {
            mViewTreeObserver.removeOnGlobalLayoutListener(this);
            mViewTreeObserver.removeOnPreDrawListener(this);
        }
        mViewTreeObserver = null;
        mObserver = null;
    }

    // ViewTreeObserver.OnGlobalLayoutListener implementation.
    @Override
    public void onGlobalLayout() {
        if (!mView.isShown()) mObserver.onRectHidden();
    }

    // ViewTreeObserver.OnPreDrawListener implementation.
    @Override
    public boolean onPreDraw() {
        if (!mView.isShown()) {
            mObserver.onRectHidden();
        } else {
            refreshAnchorBounds();
        }

        return true;
    }

    // View.OnAttachStateChangedObserver implementation.
    @Override
    public void onViewAttachedToWindow(View v) {}

    @Override
    public void onViewDetachedFromWindow(View v) {
        mObserver.onRectHidden();
    }

    private void refreshAnchorBounds() {
        mView.getLocationInWindow(mCachedWindowCoordinates);
        if (mCachedWindowCoordinates[0] < 0 || mCachedWindowCoordinates[1] < 0) return;

        mRect.left = mCachedWindowCoordinates[0];
        mRect.top = mCachedWindowCoordinates[1];
        mRect.right = mRect.left + mView.getWidth();
        mRect.bottom = mRect.top + mView.getHeight();

        mRect.left += mInsetRect.left;
        mRect.top += mInsetRect.top;
        mRect.right -= mInsetRect.right;
        mRect.bottom -= mInsetRect.bottom;

        // Account for the padding.
        boolean isRtl = ApiCompatibilityUtils.isLayoutRtl(mView);
        mRect.left += isRtl ? ApiCompatibilityUtils.getPaddingEnd(mView)
                            : ApiCompatibilityUtils.getPaddingStart(mView);
        mRect.right -= isRtl ? ApiCompatibilityUtils.getPaddingStart(mView)
                             : ApiCompatibilityUtils.getPaddingEnd(mView);
        mRect.top += mView.getPaddingTop();
        mRect.bottom -= mView.getPaddingBottom();

        // Make sure we still have a valid Rect after applying the inset.
        mRect.right = Math.max(mRect.left, mRect.right);
        mRect.bottom = Math.max(mRect.top, mRect.bottom);

        mObserver.onRectChanged(mRect);
    }
}
