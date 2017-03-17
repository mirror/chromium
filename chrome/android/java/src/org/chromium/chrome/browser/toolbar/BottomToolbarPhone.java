// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.support.v7.widget.Toolbar;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetObserver;

/**
 * Phone specific toolbar that exists at the bottom of the screen.
 */
public class BottomToolbarPhone extends ToolbarPhone implements BottomSheetObserver {
    /** The white version of the toolbar handle; used for dark themes and incognito. */
    private final Bitmap mHandleLight;

    /** The dark version of the toolbar handle; this is the default handle to use. */
    private final Bitmap mHandleDark;

    /** A handle to the bottom sheet. */
    private BottomSheet mBottomSheet;

    /**
     * Whether the end toolbar buttons should be hidden regardless of whether the URL bar is
     * focused.
     */
    private boolean mShouldHideEndToolbarButtons;

    /**
     * This tracks the height fraction of the bottom bar to determine if it is moving up or down.
     */
    private float mLastHeightFraction;

    /** The toolbar handle view that indicates the toolbar can be pulled upward. */
    private ImageView mToolbarHandleView;

    /**
     * Constructs a BottomToolbarPhone object.
     * @param context The Context in which this View object is created.
     * @param attrs The AttributeSet that was specified with this View.
     */
    public BottomToolbarPhone(Context context, AttributeSet attrs) {
        super(context, attrs);

        int defaultHandleColor =
                ApiCompatibilityUtils.getColor(getResources(), R.color.google_grey_500);
        mHandleDark = generateHandleBitmap(defaultHandleColor);

        int lightHandleColor =
                ApiCompatibilityUtils.getColor(getResources(), R.color.semi_opaque_white);
        mHandleLight = generateHandleBitmap(lightHandleColor);
    }

    @Override
    protected int getProgressBarTopMargin() {
        // In the case where the toolbar is at the bottom of the screen, the progress bar should
        // be at the top of the screen.
        return 0;
    }

    @Override
    protected void triggerUrlFocusAnimation(final boolean hasFocus) {
        super.triggerUrlFocusAnimation(hasFocus);

        if (mBottomSheet == null || !hasFocus) return;

        mBottomSheet.setSheetState(BottomSheet.SHEET_STATE_FULL, true);
    }

    @Override
    public void setBottomSheet(BottomSheet sheet) {
        assert mBottomSheet == null;

        mBottomSheet = sheet;
        getLocationBar().setBottomSheet(mBottomSheet);
        mBottomSheet.addObserver(this);
    }

    @Override
    public boolean shouldIgnoreSwipeGesture() {
        // Only detect swipes if the bottom sheet in the peeking state and not animating.
        return mBottomSheet.getSheetState() != BottomSheet.SHEET_STATE_PEEK
                || mBottomSheet.isRunningSettleAnimation() || super.shouldIgnoreSwipeGesture();
    }

    @Override
    protected void addProgressBarToHierarchy() {
        if (mProgressBar == null) return;

        ViewGroup coordinator = (ViewGroup) getRootView().findViewById(R.id.coordinator);
        coordinator.addView(mProgressBar);
        mProgressBar.setProgressBarContainer(coordinator);
    }

    /**
     * @return The extra top margin that should be applied to the browser controls views to
     *         correctly offset them from the handle that sits above them.
     */
    private int getExtraTopMargin() {
        return getResources().getDimensionPixelSize(R.dimen.bottom_toolbar_top_margin);
    }

    @Override
    public void onFinishInflate() {
        super.onFinishInflate();

        // Programmatically apply a top margin to all the children of the toolbar container. This
        // is done so the view hierarchy does not need to be changed.
        int topMarginForControls = getExtraTopMargin();

        View topShadow = findViewById(R.id.bottom_toolbar_shadow);

        for (int i = 0; i < getChildCount(); i++) {
            View curView = getChildAt(i);

            // Skip the shadow that sits at the top of the toolbar since this needs to sit on top
            // of the toolbar.
            if (curView == topShadow) continue;

            ((MarginLayoutParams) curView.getLayoutParams()).topMargin = topMarginForControls;
        }
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        // The toolbar handle is part of the control container so it can draw on top of the other
        // toolbar views. Get the root view and search for the handle.
        mToolbarHandleView = (ImageView) getRootView().findViewById(R.id.toolbar_handle);
        mToolbarHandleView.setImageBitmap(mHandleDark);
    }

    @Override
    protected void updateVisualsForToolbarState() {
        super.updateVisualsForToolbarState();

        // The handle should not show in tab switcher mode.
        mToolbarHandleView.setVisibility(
                mTabSwitcherState != ToolbarPhone.STATIC_TAB ? View.INVISIBLE : View.VISIBLE);
        mToolbarHandleView.setImageBitmap(mUseLightToolbarDrawables ? mHandleLight : mHandleDark);
    }

    @Override
    protected void updateLocationBarBackgroundBounds(Rect out, VisualState visualState) {
        super.updateLocationBarBackgroundBounds(out, visualState);

        // Allow the location bar to expand to the full height of the control container.
        out.top -= getExtraTopMargin() * mUrlExpansionPercent;
    }

    /**
     * Generate the bitmap used as the handle on the toolbar. This indicates that the toolbar can
     * be pulled up.
     * @return The handle as a bitmap.
     */
    private Bitmap generateHandleBitmap(int handleColor) {
        int handleWidth = getResources().getDimensionPixelSize(R.dimen.toolbar_handle_width);
        int handleHeight = getResources().getDimensionPixelSize(R.dimen.toolbar_handle_height);

        Bitmap handle = Bitmap.createBitmap(handleWidth, handleHeight, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(handle);

        // Clear the canvas to be completely transparent.
        canvas.drawARGB(0, 0, 0, 0);

        Paint paint = new Paint();
        paint.setColor(handleColor);
        paint.setAntiAlias(true);

        RectF rect = new RectF(0, 0, handleWidth, handleHeight);

        // Use height / 2 for the corner radius so the handle always takes the shape of a pill.
        canvas.drawRoundRect(rect, handleHeight / 2f, handleHeight / 2f, paint);

        return handle;
    }

    @Override
    protected boolean shouldHideEndToolbarButtons() {
        return mShouldHideEndToolbarButtons;
    }

    @Override
    public void onSheetOpened() {}

    @Override
    public void onSheetClosed() {}

    @Override
    public void onLoadUrl(String url) {}

    @Override
    public void onTransitionPeekToHalf(float transitionFraction) {
        // TODO(twellington): animate end toolbar button appearance/disappearance.
        if (transitionFraction >= 0.5 && !mShouldHideEndToolbarButtons) {
            mShouldHideEndToolbarButtons = true;
            updateUrlExpansionAnimation();
        } else if (transitionFraction < 0.5 && mShouldHideEndToolbarButtons) {
            mShouldHideEndToolbarButtons = false;
            updateUrlExpansionAnimation();
        }

        boolean buttonsClickable = transitionFraction == 0.f;
        mToggleTabStackButton.setClickable(buttonsClickable);
        mMenuButton.setClickable(buttonsClickable);
    }

    @Override
    public void onSheetOffsetChanged(float heightFraction) {
        boolean isMovingDown = heightFraction < mLastHeightFraction;
        mLastHeightFraction = heightFraction;

        // The only time the omnibox should have focus is when the sheet is fully expanded. Any
        // movement of the sheet should unfocus it.
        if (isMovingDown && getLocationBar().isUrlBarFocused()) {
            getLocationBar().setUrlBarFocus(false);
        }
    }

    /**
     * Sets the height and title text appearance of the provided toolbar so that its style is
     * consistent with BottomToolbarPhone.
     * @param otherToolbar The other {@link Toolbar} to style.
     */
    public void setOtherToolbarStyle(Toolbar otherToolbar) {
        // Android's Toolbar class typically changes its height based on device orientation.
        // BottomToolbarPhone has a fixed height. Update |toolbar| to match.
        otherToolbar.getLayoutParams().height = getHeight();

        // Android Toolbar action buttons are aligned based on the minimum height.
        int extraTopMargin = getExtraTopMargin();
        otherToolbar.setMinimumHeight(getHeight() - extraTopMargin);

        otherToolbar.setTitleTextAppearance(
                otherToolbar.getContext(), R.style.BottomSheetContentTitle);
        ApiCompatibilityUtils.setPaddingRelative(otherToolbar,
                ApiCompatibilityUtils.getPaddingStart(otherToolbar),
                otherToolbar.getPaddingTop() + extraTopMargin,
                ApiCompatibilityUtils.getPaddingEnd(otherToolbar), otherToolbar.getPaddingBottom());

        otherToolbar.requestLayout();
    }
}
