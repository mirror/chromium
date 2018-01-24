// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.popups;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.PopupWindow;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.content.browser.PositionObserver;
import org.chromium.content.browser.ViewPositionObserver;

/**
 * Created by twellington on 1/23/18.
 */
public class ViewAnchoredPopupWindow extends AnchoredPopupWindow
        implements PositionObserver.Listener, ViewTreeObserver.OnGlobalLayoutListener,
                   View.OnAttachStateChangeListener, ViewTreeObserver.OnPreDrawListener,
                   PopupWindow.OnDismissListener {
    private final int[] mCachedWindowCoordinates = new int[2];
    private final Rect mAnchorRect = new Rect();
    private final Rect mInsetRect = new Rect();
    private final View mAnchorView;

    private final ViewPositionObserver mViewPositionObserver;

    /** If not {@code null}, the {@link ViewTreeObserver} that we are registered to. */
    private ViewTreeObserver mViewTreeObserver;

    /**
     * Creates an instance of a {@link ViewAnchoredPopupWindow}.
     * @param context    Context to draw resources from.
     * @param anchorView The {@link View} to anchor to.
     */
    public ViewAnchoredPopupWindow(Context context, View anchorView, Drawable background,
            View contentView, int marginPx, int width) {
        super(context, anchorView.getRootView(), background, contentView, marginPx, width);
        mAnchorView = anchorView;

        mViewPositionObserver = new ViewPositionObserver(mAnchorView);
    }

    /**
     * Specifies the inset values in pixels that determine how to shrink the {@link View} bounds
     * when creating the anchor {@link Rect}.
     */
    public void setInsetPx(int left, int top, int right, int bottom) {
        mInsetRect.set(left, top, right, bottom);
        refreshAnchorBounds();
    }

    // TextBubble implementation.
    @Override
    public void show() {
        mViewPositionObserver.addListener(this);
        mAnchorView.addOnAttachStateChangeListener(this);
        mViewTreeObserver = mAnchorView.getViewTreeObserver();
        mViewTreeObserver.addOnGlobalLayoutListener(this);
        mViewTreeObserver.addOnPreDrawListener(this);

        refreshAnchorBounds();
        super.show();
    }

    @Override
    public void onDismiss() {
        mViewPositionObserver.removeListener(this);
        mAnchorView.removeOnAttachStateChangeListener(this);

        if (mViewTreeObserver != null && mViewTreeObserver.isAlive()) {
            mViewTreeObserver.removeOnGlobalLayoutListener(this);
            mViewTreeObserver.removeOnPreDrawListener(this);
        }
        mViewTreeObserver = null;
    }

    // ViewTreeObserver.OnGlobalLayoutListener implementation.
    @Override
    public void onGlobalLayout() {
        if (!mAnchorView.isShown()) dismiss();
    }

    // ViewTreeObserver.OnPreDrawListener implementation.
    @Override
    public boolean onPreDraw() {
        if (!mAnchorView.isShown()) dismiss();
        return true;
    }

    // View.OnAttachStateChangedObserver implementation.
    @Override
    public void onViewAttachedToWindow(View v) {}

    @Override
    public void onViewDetachedFromWindow(View v) {
        dismiss();
    }

    // TODO(twellington): Try removing PositionObserver and ViewPositionObserver since DEPS
    //                    complains? Or move these widgets to content/?
    // PositionObserver.Listener implementation.
    @Override
    public void onPositionChanged(int positionX, int positionY) {
        refreshAnchorBounds();
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

        setAnchorRect(mAnchorRect);
    }
}
