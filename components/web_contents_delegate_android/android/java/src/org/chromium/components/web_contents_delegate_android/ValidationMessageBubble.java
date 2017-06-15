// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.web_contents_delegate_android;

import android.graphics.Point;
import android.graphics.RectF;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.CalledByNative;

/**
 * This class is an implementation of validation message bubble UI.
 */
class ValidationMessageBubble {
    private PopupWindow mPopup;

    /**
     * Creates a popup window to show the specified messages, and show it on the specified anchor
     * rectangle.
     *
     * If the anchor view is not in a state where a popup can be shown, this will return null.
     *
     * @param containerView
     * @param viewportWidth
     * @param viewportHeight
     * @param contentOffsetY
     * @param anchorX Anchor position in the pixel unit.
     * @param anchorY Anchor position in the device unit.
     * @param anchorWidth Anchor size in the device unit.
     * @param anchorHeight Anchor size in the device unit.
     * @param mainText The main message. It will shown at the top of the popup window, and its font
     *                 size is larger.
     * @param subText The sub message. It will shown below the main message, and its font size is
     *                smaller.
     */
    @CalledByNative
    private static ValidationMessageBubble createAndShowIfApplicable(View containerView,
            int viewportWidth, int viewportHeight, float contentOffsetY, int anchorX, int anchorY,
            int anchorWidth, int anchorHeight, String mainText, String subText) {
        if (!canShowBubble(containerView)) return null;

        RectF anchorPixInScreen = makePixRectInScreen(
                containerView, contentOffsetY, anchorX, anchorY, anchorWidth, anchorHeight);
        return new ValidationMessageBubble(containerView, viewportWidth, viewportHeight,
                contentOffsetY, anchorPixInScreen, mainText, subText);
    }

    private static boolean canShowBubble(View containerView) {
        return containerView != null && containerView.getWindowToken() != null;
    }

    private ValidationMessageBubble(View containerView, int viewportWidth, int viewportHeight,
            float contentOffsetY, RectF anchor, String mainText, String subText) {
        final ViewGroup root = (ViewGroup) View.inflate(
                containerView.getContext(), R.layout.validation_message_bubble, null);
        mPopup = new PopupWindow(root);
        updateTextViews(root, mainText, subText);
        measure(viewportWidth, viewportHeight);
        Point origin = adjustWindowPosition(containerView, viewportWidth, viewportHeight,
                contentOffsetY, (int) (anchor.centerX() - getAnchorOffset()), (int) anchor.bottom);
        mPopup.showAtLocation(containerView, Gravity.NO_GRAVITY, origin.x, origin.y);
    }

    @CalledByNative
    private void close() {
        if (mPopup == null) return;
        mPopup.dismiss();
        mPopup = null;
    }

    /**
     * Moves the popup window on the specified anchor rectangle.
     *
     * @param containerView
     * @param viewportWidth
     * @param viewportHeight
     * @param contentOffsetY
     * @param anchorX Anchor position in the device unit.
     * @param anchorY Anchor position in the device unit.
     * @param anchorWidth Anchor size in the device unit.
     * @param anchorHeight Anchor size in the device unit.
     */
    @CalledByNative
    private void setPositionRelativeToAnchor(View containerView, int viewportWidth,
            int viewportHeight, float contentOffsetY, int anchorX, int anchorY, int anchorWidth,
            int anchorHeight) {
        RectF anchor = makePixRectInScreen(
                containerView, contentOffsetY, anchorX, anchorY, anchorWidth, anchorHeight);
        Point origin = adjustWindowPosition(containerView, viewportWidth, viewportHeight,
                contentOffsetY, (int) (anchor.centerX() - getAnchorOffset()), (int) anchor.bottom);
        mPopup.update(origin.x, origin.y, mPopup.getWidth(), mPopup.getHeight());
    }

    private static RectF makePixRectInScreen(View containerView, float contentOffsetYPix,
            int anchorX, int anchorY, int anchorWidth, int anchorHeight) {
        final float yOffset = getWebViewOffsetYPixInScreen(containerView, contentOffsetYPix);
        return new RectF(anchorX, anchorY + yOffset, anchorX + anchorWidth,
                anchorY + anchorHeight + yOffset);
    }

    private static float getWebViewOffsetYPixInScreen(View containerView, float contentOffsetYPix) {
        int[] location = new int[2];
        containerView.getLocationOnScreen(location);
        return location[1] + contentOffsetYPix;
    }

    private static void updateTextViews(ViewGroup root, String mainText, String subText) {
        ((TextView) root.findViewById(R.id.main_text)).setText(mainText);
        final TextView subTextView = (TextView) root.findViewById(R.id.sub_text);
        if (!TextUtils.isEmpty(subText)) {
            subTextView.setText(subText);
        } else {
            ((ViewGroup) subTextView.getParent()).removeView(subTextView);
        }
    }

    private void measure(int viewportWidth, int viewportHeight) {
        mPopup.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopup.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopup.getContentView().setLayoutParams(
                new RelativeLayout.LayoutParams(
                        RelativeLayout.LayoutParams.WRAP_CONTENT,
                        RelativeLayout.LayoutParams.WRAP_CONTENT));
        mPopup.getContentView().measure(
                View.MeasureSpec.makeMeasureSpec(viewportWidth, View.MeasureSpec.AT_MOST),
                View.MeasureSpec.makeMeasureSpec(viewportHeight, View.MeasureSpec.AT_MOST));
    }

    private float getAnchorOffset() {
        final View root = mPopup.getContentView();
        final int width = root.getMeasuredWidth();
        final int arrowWidth = root.findViewById(R.id.arrow_image).getMeasuredWidth();
        return ApiCompatibilityUtils.isLayoutRtl(root)
                ? (width * 3 / 4 - arrowWidth / 2) : (width / 4 + arrowWidth / 2);
    }

    /**
     * This adjusts the position if the popup protrudes the web view.
     */
    private Point adjustWindowPosition(
            View containerView, int viewWidth, int viewHeight, float contentOffsetY, int x, int y) {
        final int viewBottom =
                (int) getWebViewOffsetYPixInScreen(containerView, contentOffsetY) + viewHeight;
        final int width = mPopup.getContentView().getMeasuredWidth();
        final int height = mPopup.getContentView().getMeasuredHeight();
        if (x < 0) {
            x = 0;
        } else if (x + width > viewWidth) {
            x = viewWidth - width;
        }
        if (y + height > viewBottom) {
            y = viewBottom - height;
        }
        return new Point(x, y);
    }
}
