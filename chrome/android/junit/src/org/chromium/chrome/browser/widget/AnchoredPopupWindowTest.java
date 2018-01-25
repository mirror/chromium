// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import static org.junit.Assert.assertEquals;

import android.graphics.Rect;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for the static positioning methods in {@link AnchoredPopupWindow}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public final class AnchoredPopupWindowTest {
    private Rect mWindowRect;
    private Rect mBackgroundPaddingRect;
    int mPopupWidth;
    int mPopupHeight;

    @Before
    public void setUp() {
        mWindowRect = new Rect(0, 0, 600, 1000);
        mBackgroundPaddingRect = new Rect();
        mPopupWidth = 150;
        mPopupHeight = 300;
    }

    @Test
    public void getPopupPosition_BelowRight() {
        Rect anchorRect = new Rect(10, 10, 20, 20);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 0, false,
                mBackgroundPaddingRect,
                AnchoredPopupWindow.HORIZONTAL_ORIENTATION_MAX_AVAILABLE_SPACE);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, false, mBackgroundPaddingRect, true);

        assertEquals(20, x);
        assertEquals(20, y);
    }

    @Test
    public void getPopupPosition_BelowRight_Overlap() {
        Rect anchorRect = new Rect(10, 10, 20, 20);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 0, true,
                mBackgroundPaddingRect,
                AnchoredPopupWindow.HORIZONTAL_ORIENTATION_MAX_AVAILABLE_SPACE);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, true, mBackgroundPaddingRect, true);

        assertEquals(10, x);
        assertEquals(10, y);
    }

    @Test
    public void getPopupPosition_BelowCenter() {
        Rect anchorRect = new Rect(295, 10, 305, 20);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 0, false,
                mBackgroundPaddingRect, AnchoredPopupWindow.HORIZONTAL_ORIENTATION_CENTER);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, false, mBackgroundPaddingRect, true);

        assertEquals(225, x);
        assertEquals(20, y);
    }

    @Test
    public void getPopupPosition_AboveLeft() {
        Rect anchorRect = new Rect(400, 800, 410, 820);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 0, false,
                mBackgroundPaddingRect,
                AnchoredPopupWindow.HORIZONTAL_ORIENTATION_MAX_AVAILABLE_SPACE);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, false, mBackgroundPaddingRect, false);

        assertEquals(250, x);
        assertEquals(500, y);
    }

    @Test
    public void getPopupPosition_AboveLeft_Overlap() {
        Rect anchorRect = new Rect(400, 800, 410, 820);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 0, true,
                mBackgroundPaddingRect,
                AnchoredPopupWindow.HORIZONTAL_ORIENTATION_MAX_AVAILABLE_SPACE);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, true, mBackgroundPaddingRect, false);

        assertEquals(260, x);
        assertEquals(520, y);
    }

    @Test
    public void getPopupPosition_ClampedLeftEdge() {
        Rect anchorRect = new Rect(10, 10, 20, 20);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 20, false,
                mBackgroundPaddingRect,
                AnchoredPopupWindow.HORIZONTAL_ORIENTATION_MAX_AVAILABLE_SPACE);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, true, mBackgroundPaddingRect, true);

        assertEquals(20, x);
    }

    @Test
    public void getPopupPosition_ClampedRightEdge() {
        Rect anchorRect = new Rect(590, 800, 600, 820);
        int x = AnchoredPopupWindow.getPopupX(anchorRect, mWindowRect, mPopupWidth, 20, false,
                mBackgroundPaddingRect,
                AnchoredPopupWindow.HORIZONTAL_ORIENTATION_MAX_AVAILABLE_SPACE);
        int y = AnchoredPopupWindow.getPopupY(
                anchorRect, mPopupHeight, true, mBackgroundPaddingRect, true);

        assertEquals(430, x);
    }
}