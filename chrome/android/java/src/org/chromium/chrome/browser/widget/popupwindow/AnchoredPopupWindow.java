// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.popupwindow;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnPreDrawListener;
import android.widget.FrameLayout;
import android.widget.PopupWindow;
import android.widget.PopupWindow.OnDismissListener;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ObserverList;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.content.browser.PositionObserver;
import org.chromium.content.browser.ViewPositionObserver;

/**
 * ...
 */
public class AnchoredPopupWindow implements OnTouchListener, PositionObserver.Listener,
        ViewTreeObserver.OnGlobalLayoutListener, View.OnAttachStateChangeListener,
        OnPreDrawListener, OnDismissListener {
    public enum Orientation { MAX_AVAILABLE_SPACE, BELOW, ABOVE }

    // Cache Rect objects for querying View and Screen coordinate APIs.
    protected Rect mCachedPaddingRect = new Rect();
    protected Rect mCachedWindowRect = new Rect();

    private Context mContext;
    private View mRootView;

    /** The margin to add to the text bubble so it doesn't bump against the edges of the screen. */
    protected int mMarginPx;

    /** The actual {@link PopupWindow}.  Internalized to prevent API leakage. */
    private PopupWindow mPopupWindow;

    /** The {@link Drawable} that is responsible for drawing the bubble and the arrow. */
    private Drawable mDrawable;

    /** The {@link Rect} to anchor the bubble to in screen space. */
    protected Rect mAnchorRect = new Rect();

    private final OnDismissListener mDismissListener = new OnDismissListener() {
        @Override
        public void onDismiss() {
            if (mIgnoreDismissal) return;

            for (OnDismissListener listener : mDismissListeners) listener.onDismiss();
        }
    };

    // Pass through for the internal PopupWindow.  This class needs to intercept these for API
    // purposes, but they are still useful to callers.
    private ObserverList<OnDismissListener> mDismissListeners = new ObserverList<>();
    private OnTouchListener mTouchListener;
    private boolean mDismissOnTouchInteraction;

    // Positioning/sizing coordinates for the popup bubble.
    protected int mX;
    protected int mY;
    protected int mWidth;
    protected int mHeight;

    // Preferred orientation for the bubble with respect to the anchor.
    private Orientation mPreferredOrientation = Orientation.MAX_AVAILABLE_SPACE;

    /**
     * Tracks whether or not we are in the process of updating the bubble, which might include a
     * dismiss and show.  In that case we don't want to let the world know we're dismissing because
     * it's only temporary.
     */
    private boolean mIgnoreDismissal;

    private int mMenuWidth;
    private boolean mOverlapAnchor;
    protected boolean mCurrentPositionBelow;

    // Provide view anchoring!
    private int[] mCachedWindowCoordinates = new int[2];
    private Rect mInsetRect = new Rect();
    private View mAnchorView;

    private ViewPositionObserver mViewPositionObserver;

    /** If not {@code null}, the {@link ViewTreeObserver} that we are registered to. */
    private ViewTreeObserver mViewTreeObserver;


    public AnchoredPopupWindow() {}
    /**
     * Constructs a {@link AnchoredPopupWindow} instance.
     * @param context  Context to draw resources from.
     * @param rootView The {@link View} to use for size calculations and for display.
     * @param background
     */
    public void initialize(Context context, View rootView, Drawable background, int menuWidth,
            boolean overlapAnchor, View contentView) {
        mContext = context;
        mRootView = rootView.getRootView();
        mPopupWindow = new PopupWindow(mContext);
        mMenuWidth = menuWidth;
        mOverlapAnchor = overlapAnchor;

        mDrawable = background;

        mHandler = new Handler();

        mPopupWindow.setWidth(menuWidth);
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(mDrawable);

        mPopupWindow.setContentView(contentView);

        // On some versions of Android, the LayoutParams aren't set until after the popup window
        // is shown. Explicitly set the LayoutParams to avoid crashing. See crbug.com/713759.
        contentView.setLayoutParams(
                new FrameLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

        mPopupWindow.setTouchInterceptor(this);
        mPopupWindow.setOnDismissListener(mDismissListener);
        setDismissOnTouchInteraction(true);
    }

    public void setAnchorView(View anchorView) {
        mAnchorView = anchorView;

        mViewPositionObserver = new ViewPositionObserver(mAnchorView);
    }

    public PopupWindow getPopupWindow() {
        return mPopupWindow;
    }

    public void setMargin(int margin) {
        mMarginPx = margin;
    }

    public void setWidth(int width) {
        mMenuWidth = width;
    }

    /** Shows the bubble. Will have no effect if the bubble is already showing. */
    public void show() {
        if (mAnchorView != null) {
            mViewPositionObserver.addListener(this);
            mAnchorView.addOnAttachStateChangeListener(this);
            mViewTreeObserver = mAnchorView.getViewTreeObserver();
            mViewTreeObserver.addOnGlobalLayoutListener(this);
            mViewTreeObserver.addOnPreDrawListener(this);

            refreshAnchorBounds();
        }

        if (mPopupWindow.isShowing()) return;

        if (!mPopupWindow.isShowing()) {
            mPopupWindow.dismiss();
        }

        updatePopupLayout();
        mPopupWindow.showAtLocation(mRootView, Gravity.TOP | Gravity.START, mX, mY);
    }

    /**
     * Disposes of the bubble.  Will have no effect if the bubble isn't showing.
     * @see PopupWindow#dismiss()
     */
    public void dismiss() {
        mPopupWindow.dismiss();
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

    /**
     * @return Whether the bubble is currently showing.
     */
    public boolean isShowing() {
        return mPopupWindow.isShowing();
    }

    /**
     * @param onTouchListener A callback for all touch events being dispatched to the bubble.
     * @see PopupWindow#setTouchInterceptor(OnTouchListener)
     */
    public void setTouchInterceptor(OnTouchListener onTouchListener) {
        mTouchListener = onTouchListener;
    }

    /**
     * @param onDismissListener A listener to be called when the bubble is dismissed.
     * @see PopupWindow#setOnDismissListener(OnDismissListener)
     */
    public void addOnDismissListener(OnDismissListener onDismissListener) {
        mDismissListeners.addObserver(onDismissListener);
    }

    /**
     * @param onDismissListener The listener to remove and not call when the bubble is dismissed.
     * @see PopupWindow#setOnDismissListener(OnDismissListener)
     */
    public void removeOnDismissListener(OnDismissListener onDismissListener) {
        mDismissListeners.removeObserver(onDismissListener);
    }

    /**
     * @param dismiss Whether or not to dismiss this bubble when the screen is tapped.  This will
     *                happen for both taps inside and outside the popup.  The default is
     *                {@code false}.
     */
    public void setDismissOnTouchInteraction(boolean dismiss) {
        mDismissOnTouchInteraction = dismiss;
        mPopupWindow.setOutsideTouchable(mDismissOnTouchInteraction);
    }

    /**
     * Sets the {@link Rect} this bubble attaches and orients to.  This will move the bubble if it
     * is already visible, but should be called at least once calling {@link #show()}.
     *
     * @param rect A {@link Rect} object that represents the anchor object's position in screen
     *             space (e.g. if this bubble is backing a {@link View}, see
     *             {@link View#getLocationOnScreen(int[])}.
     */
    public void setAnchorRect(Rect rect) {
        mAnchorRect.set(rect);
        if (mPopupWindow.isShowing()) updatePopupLayout();
    }

    /**
     * Sets the preferred orientation of the bubble with respect to the anchor view such as above or
     * below the anchor.
     * @param orientation The orientation preferred.
     */
    public void setPreferredOrientation(Orientation orientation) {
        mPreferredOrientation = orientation;
    }

    /**
     * Causes this bubble to position/size itself.  The calculations will happen even if the bubble
     * isn't visible.
     */
    protected void updatePopupLayout() {
        // Determine the size of the text bubble.
        Drawable background = mPopupWindow.getBackground();
        boolean currentPositionBelow = mCurrentPositionBelow;
        boolean preferCurrentOrientation = mPopupWindow.isShowing();

        background.getPadding(mCachedPaddingRect);
        int paddingX = mCachedPaddingRect.left + mCachedPaddingRect.right;
        int paddingY = mCachedPaddingRect.top + mCachedPaddingRect.bottom;

        // Determine whether or not the bubble should be above or below the anchor.
        // Aggressively try to put it below the anchor.  Put it above only if it would fit better.
        View contentView = mPopupWindow.getContentView();
        int widthSpec = MeasureSpec.makeMeasureSpec(mMenuWidth, MeasureSpec.AT_MOST);
        contentView.measure(widthSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
        int idealHeight = contentView.getMeasuredHeight();

        mRootView.getWindowVisibleDisplayFrame(mCachedWindowRect);

        // In multi-window, the coordinates of root view will be different than (0,0).
        // So we translate the coordinates of |mCachedWindowRect| w.r.t. its window.
        int[] rootCoordinates = new int[2];
        mRootView.getLocationOnScreen(rootCoordinates);
        mCachedWindowRect.offset(-rootCoordinates[0], -rootCoordinates[1]);

        // TODO(dtrainor): This follows the previous logic.  But we should look into if we want to
        // use the root view dimensions instead of the window dimensions here so the bubble can't
        // bleed onto the decorations.
        int spaceAboveAnchor = mAnchorRect.top - mCachedWindowRect.top - paddingY - mMarginPx;
        int spaceBelowAnchor = mCachedWindowRect.bottom - mAnchorRect.bottom - paddingY- mMarginPx;

        // Bias based on the center of the bubble and where it is on the screen.
        boolean idealFitsBelow = idealHeight <= spaceBelowAnchor;
        boolean idealFitsAbove = idealHeight <= spaceAboveAnchor;

        // Position the bubble in the largest available space where it can fit.  This will bias the
        // bubbles to show below the anchor if it will not fit in either place.
        boolean positionBelow =
                (idealFitsBelow && spaceBelowAnchor >= spaceAboveAnchor) || !idealFitsAbove;

        // Override the ideal bubble orientation if we are trying to maintain the current one.
        if (preferCurrentOrientation && currentPositionBelow != positionBelow) {
            if (currentPositionBelow && idealFitsBelow) positionBelow = true;
            if (!currentPositionBelow && idealFitsAbove) positionBelow = false;
        }

        if (mPreferredOrientation == Orientation.BELOW && idealFitsBelow) positionBelow = true;
        if (mPreferredOrientation == Orientation.ABOVE && idealFitsAbove) positionBelow = false;

        int maxContentHeight = positionBelow ? spaceBelowAnchor : spaceAboveAnchor;
        contentView.measure(
                widthSpec, MeasureSpec.makeMeasureSpec(maxContentHeight, MeasureSpec.AT_MOST));

        mWidth = contentView.getMeasuredWidth() + paddingX;
        mHeight = contentView.getMeasuredHeight() + paddingY;

        int spaceToLeftOfAnchor = mAnchorRect.left - mCachedWindowRect.left;
        int spaceToRightOfAnchor = mCachedWindowRect.right - mAnchorRect.right;
        boolean idealFitsToLeft = mWidth <= spaceToLeftOfAnchor;
        boolean idealFitsToRight = mWidth <= spaceToRightOfAnchor;

        boolean positionToLeft = (idealFitsToLeft && spaceToLeftOfAnchor >= spaceToRightOfAnchor)
                || !idealFitsToRight;
        Rect bgPadding = new Rect();
        mPopupWindow.getBackground().getPadding(bgPadding);

        // Determine the position of the popup.
        mY = getPopupY(positionBelow, bgPadding);
        mX = getPopupX(positionToLeft, bgPadding);

        // In landscape mode, root view includes the decorations in some devices. So we guard the
        // window dimensions against |mCachedWindowRect.right| instead.
        mX = MathUtils.clamp(mX, mMarginPx, mCachedWindowRect.right - mWidth - mMarginPx);

        mCurrentPositionBelow = positionBelow;

        if (mPopupWindow.isShowing() && positionBelow != currentPositionBelow) {
            // This is a hack to deal with the case where the arrow flips between top and bottom.
            // In this case the padding of the background drawable in the PopupWindow changes.
            try {
                mIgnoreDismissal = true;
                mPopupWindow.dismiss();
                mPopupWindow.showAtLocation(mRootView, Gravity.TOP | Gravity.START, mX, mY);
            } finally {
                mIgnoreDismissal = false;
            }
        }

        mPopupWindow.update(mX, mY, mWidth, mHeight);
        mPopupWindow.setAnimationStyle(getPopupAnimation(positionBelow));
    }

    public int getPopupY(boolean positionBelow, Rect bgPadding) {
        if (positionBelow) {
            return mOverlapAnchor ? mAnchorRect.top : mAnchorRect.bottom;
        } else {
           return mOverlapAnchor ? mAnchorRect.bottom + bgPadding.bottom - mHeight
                   : mAnchorRect.top;
        }
    }

    public int getPopupX(boolean positionToLeft, Rect bgPadding) {
        if (positionToLeft) {
            return mAnchorRect.right - mWidth + bgPadding.right;
        } else {
            return mAnchorRect.left - bgPadding.left;
        }
    }

    public int getPopupAnimation(boolean positionBelow) {
        return positionBelow ? R.style.OverflowMenuAnim : R.style.OverflowMenuAnimBottom;
    }

    /**
     * Specifies the inset values in pixels that determine how to shrink the {@link View} bounds
     * when creating the anchor {@link Rect}.
     */
    public void setInsetPx(int left, int top, int right, int bottom) {
        mInsetRect.set(left, top, right, bottom);
        refreshAnchorBounds();
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

    // OnTouchListener implementation.
    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouch(View v, MotionEvent event) {
        boolean returnValue = mTouchListener != null && mTouchListener.onTouch(v, event);
        if (mDismissOnTouchInteraction) dismiss();
        return returnValue;
    }
}
