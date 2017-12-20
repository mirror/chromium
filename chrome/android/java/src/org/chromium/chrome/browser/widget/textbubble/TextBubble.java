// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.textbubble;

import android.content.Context;
import android.graphics.Rect;
import android.os.Handler;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.PopupWindow.OnDismissListener;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.AccessibilityUtil;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.widget.popupwindow.AnchoredPopupWindow;

import java.util.HashSet;
import java.util.Set;

/**
 * UI component that handles showing a text callout bubble.  Positioning this bubble happens through
 * calls to {@link #setAnchorRect(Rect)}.  This should be called at least once before the
 * {@link #show()} call.  To attach to a {@link View} see {@link ViewAnchoredTextBubble}.
 */
public class TextBubble extends AnchoredPopupWindow {
    /**
     * Specifies no limit to the popup duration.
     * @see #setAutoDismissTimeout(long)
     */
    public static final long NO_TIMEOUT = 0;

    /**
     * A set of bubbles which are active at this moment. This set can be used to dismiss the
     * bubbles on a back press event.
     */
    private static final Set<TextBubble> sBubbles = new HashSet<>();

    private final Context mContext;
    private final Handler mHandler;
    private final View mRootView;

    /** The {@link Drawable} that is responsible for drawing the bubble and the arrow. */
    private final ArrowBubbleDrawable mDrawable;

    private final Runnable mDismissRunnable = new Runnable() {
        @Override
        public void run() {
            if (isShowing()) dismiss();
        }
    };

    private final OnDismissListener mDismissListener = new OnDismissListener() {
        @Override
        public void onDismiss() {
            sBubbles.remove(TextBubble.this);
        }
    };

    /**
     * How long to wait before automatically dismissing the bubble.  {@link #NO_TIMEOUT} is the
     * default and means the bubble will stay visible indefinitely.
     */
    private long mAutoDismissTimeoutMs;

    // Content specific variables.
    /** The resource id for the string to show in the bubble. */
    @StringRes
    private final int mStringId;

    /** The resource id for the accessibility string associated with the bubble. */
    @StringRes
    private final int mAccessibilityStringId;

    /**
     * Constructs a {@link TextBubble} instance using the default arrow drawable background.
     * @param context  Context to draw resources from.
     * @param rootView The {@link View} to use for size calculations and for display.
     * @param stringId The id of the string resource for the text that should be shown.
     * @param accessibilityStringId The id of the string resource of the accessibility text.
     */
    public TextBubble(Context context, View rootView, @StringRes int stringId,
            @StringRes int accessibilityStringId) {
        this(context, rootView, stringId, accessibilityStringId, true);
    }

    /**
     * Constructs a {@link TextBubble} instance.
     * @param context  Context to draw resources from.
     * @param rootView The {@link View} to use for size calculations and for display.
     * @param stringId The id of the string resource for the text that should be shown.
     * @param accessibilityStringId The id of the string resource of the accessibility text.
     * @param showArrow Whether the bubble should have an arrow.
     */
    public TextBubble(Context context, View rootView, @StringRes int stringId,
            @StringRes int accessibilityStringId, boolean showArrow) {
        super();

        mContext = context;
        mRootView = rootView.getRootView();
        mStringId = stringId;
        mAccessibilityStringId = accessibilityStringId;

        mDrawable = new ArrowBubbleDrawable(context);
        mDrawable.setShowArrow(showArrow);

        mHandler = new Handler();

        int marginPx = context.getResources().getDimensionPixelSize(R.dimen.text_bubble_margin);

        // Set predefined styles for the TextBubble.
        mDrawable.setBubbleColor(
                ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.google_blue_500));

        initialize(mContext, rootView, mDrawable, 0, false, createContentView());
        int paddingX = mCachedPaddingRect.left + mCachedPaddingRect.right;
        int maxContentWidth = mRootView.getWidth() - paddingX - marginPx * 2;
        setMargin(marginPx);
        setWidth(maxContentWidth);
        addOnDismissListener(mDismissListener);
    }

    @Override
    public void show() {
        if (isShowing()) return;

        if (!isShowing() && mAutoDismissTimeoutMs != NO_TIMEOUT) {
            mHandler.postDelayed(mDismissRunnable, mAutoDismissTimeoutMs);
        }

        super.show();

        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!isShowing() || getPopupWindow().getContentView() == null) return;

                getPopupWindow().getContentView().announceForAccessibility(
                        mContext.getString(mAccessibilityStringId));
            }
        });

        sBubbles.add(this);
    }
    /**
     * Dismisses all the currently showing bubbles.
     */
    public static void dismissBubbles() {
        Set<TextBubble> bubbles = new HashSet<>(sBubbles);
        for (TextBubble bubble : bubbles) {
            bubble.dismiss();
        }
    }

    /**
     * Updates the timeout that is used to determine when to automatically dismiss the bubble.  If
     * the bubble is already showing, the timeout will start from the time of this call.  Any
     * previous timeouts will be canceled.  {@link #NO_TIMEOUT} is the default value.
     * @param timeoutMs The time (in milliseconds) the bubble should be dismissed after.  Use
     *                  {@link #NO_TIMEOUT} for no timeout.
     */
    public void setAutoDismissTimeout(long timeoutMs) {
        mAutoDismissTimeoutMs = timeoutMs;
        mHandler.removeCallbacks(mDismissRunnable);
        if (isShowing() && mAutoDismissTimeoutMs != NO_TIMEOUT) {
            mHandler.postDelayed(mDismissRunnable, mAutoDismissTimeoutMs);
        }
    }

    @Override
    protected void updatePopupLayout() {
        super.updatePopupLayout();

        int arrowXOffset = 0;
        if (mDrawable.isShowingArrow()) {
            arrowXOffset = mAnchorRect.centerX() - mX;

            // Force the anchor to be in a reasonable spot w.r.t. the bubble (not over the corners).
            int minArrowOffset = mDrawable.getArrowLeftSpacing();
            int maxArrowOffset = mWidth - mDrawable.getArrowRightSpacing();
            arrowXOffset = MathUtils.clamp(arrowXOffset, minArrowOffset, maxArrowOffset);
        }

        // TODO(dtrainor): Figure out how to move the arrow and bubble to make things look
        // better.
        mDrawable.setPositionProperties(arrowXOffset, mCurrentPositionBelow);
    }

    @Override
    public int getPopupY(boolean positionBelow, Rect bgPadding) {
        if (positionBelow) {
           return mAnchorRect.bottom;
        } else {
            return mAnchorRect.top - mHeight;
        }
    }

    @Override
    public int getPopupX(boolean positionToLeft, Rect bgPadding) {
        return mAnchorRect.left + (mAnchorRect.width() - mWidth) / 2 + mMarginPx;
    }

    @Override
    public int getPopupAnimation(boolean positionBelow) {
        return R.style.TextBubbleAnimation;
    }

    private View createContentView() {
        View view = LayoutInflater.from(mContext).inflate(R.layout.textbubble_text, null);
        ((TextView) view)
                .setText(AccessibilityUtil.isAccessibilityEnabled() ? mAccessibilityStringId
                                                                    : mStringId);
        return view;
    }
}
