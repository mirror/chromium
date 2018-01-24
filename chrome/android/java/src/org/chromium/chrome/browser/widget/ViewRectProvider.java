// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;

import org.chromium.base.ApiCompatibilityUtils;
/**
 * TODO(twellington): write this
 */
public class ViewRectProvider implements ViewTreeObserver.OnGlobalLayoutListener,
                                         View.OnAttachStateChangeListener,
                                         ViewTreeObserver.OnPreDrawListener {
    /**
     * TODO(twellington): write this
     */
    public interface ViewRectProviderObserver {
        void onRectChanged(Rect newRect);
        void onRectHidden();
    }

    private final int[] mCachedWindowCoordinates = new int[2];
    private final Rect mAnchorRect = new Rect();
    private final Rect mInsetRect = new Rect();
    private final View mAnchorView;

    /** If not {@code null}, the {@link ViewTreeObserver} that we are registered to. */
    private ViewTreeObserver mViewTreeObserver;

    private ViewRectProviderObserver mObserver;

    /**
     * Creates an instance of a {@link ViewRectProvider}.
     * @param anchorView The {@link View} used to provide a Rect... make better
     */
    public ViewRectProvider(View anchorView) {
        mAnchorView = anchorView;
    }

    /**
     * Specifies the inset values in pixels that determine how to shrink the {@link View} bounds
     * when creating the anchor {@link Rect}.
     */
    public void setInsetPx(int left, int top, int right, int bottom) {
        mInsetRect.set(left, top, right, bottom);
        if (mObserver != null) refreshAnchorBounds();
    }

    public void startObserving(ViewRectProviderObserver observer) {
        mObserver = observer;
        mAnchorView.addOnAttachStateChangeListener(this);
        mViewTreeObserver = mAnchorView.getViewTreeObserver();
        mViewTreeObserver.addOnGlobalLayoutListener(this);
        mViewTreeObserver.addOnPreDrawListener(this);

        refreshAnchorBounds();
    }

    public void stopObserving() {
        mAnchorView.removeOnAttachStateChangeListener(this);

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
        if (!mAnchorView.isShown()) mObserver.onRectHidden();
    }

    // ViewTreeObserver.OnPreDrawListener implementation.
    @Override
    public boolean onPreDraw() {
        if (!mAnchorView.isShown()) {
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
        mAnchorView.getLocationInWindow(mCachedWindowCoordinates);
        if (mCachedWindowCoordinates[0] < 0 || mCachedWindowCoordinates[1] < 0) return;

        mAnchorRect.left = mCachedWindowCoordinates[0];
        mAnchorRect.top = mCachedWindowCoordinates[1];
        mAnchorRect.right = mAnchorRect.left + mAnchorView.getWidth();
        mAnchorRect.bottom = mAnchorRect.top + mAnchorView.getHeight();

        mAnchorRect.left += mInsetRect.left;
        mAnchorRect.top += mInsetRect.top;
        mAnchorRect.right -= mInsetRect.right;
        mAnchorRect.bottom -= mInsetRect.bottom;

        // Account for the padding.
        boolean isRtl = ApiCompatibilityUtils.isLayoutRtl(mAnchorView);
        mAnchorRect.left += isRtl ? ApiCompatibilityUtils.getPaddingEnd(mAnchorView)
                                  : ApiCompatibilityUtils.getPaddingStart(mAnchorView);
        mAnchorRect.right -= isRtl ? ApiCompatibilityUtils.getPaddingStart(mAnchorView)
                                   : ApiCompatibilityUtils.getPaddingEnd(mAnchorView);
        mAnchorRect.top += mAnchorView.getPaddingTop();
        mAnchorRect.bottom -= mAnchorView.getPaddingBottom();

        // Make sure we still have a valid Rect after applying the inset.
        mAnchorRect.right = Math.max(mAnchorRect.left, mAnchorRect.right);
        mAnchorRect.bottom = Math.max(mAnchorRect.top, mAnchorRect.bottom);

        mObserver.onRectChanged(mAnchorRect);
    }
}
