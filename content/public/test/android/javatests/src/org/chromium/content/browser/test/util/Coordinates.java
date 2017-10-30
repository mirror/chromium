// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import android.content.Context;

import org.chromium.content.browser.RenderCoordinates;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content_public.browser.WebContents;

/**
 * Util class that allows tests to get various view-related coordinate values and
 * scale factors from {@link RenderCoordinates}.
 */
public class Coordinates {
    private final RenderCoordinates mRenderCoordinates;

    public static Coordinates createFor(WebContents webContents) {
        return new Coordinates(webContents);
    }

    private Coordinates(WebContents webContents) {
        mRenderCoordinates = ((WebContentsImpl) webContents).getRenderCoordinates();
    }

    public float getLastFrameViewportWidthCss() {
        return mRenderCoordinates.getLastFrameViewportWidthCss();
    }

    public float getLastFrameViewportHeightCss() {
        return mRenderCoordinates.getLastFrameViewportHeightCss();
    }

    public int getLastFrameViewportWidthPixInt() {
        return mRenderCoordinates.getLastFrameViewportWidthPixInt();
    }

    public int getLastFrameViewportHeightPixInt() {
        return mRenderCoordinates.getLastFrameViewportHeightPixInt();
    }

    public float getContentOffsetYPix() {
        return mRenderCoordinates.getContentOffsetYPix();
    }

    public float getPageScaleFactor() {
        return mRenderCoordinates.getPageScaleFactor();
    }

    public float getMinPageScaleFactor() {
        return mRenderCoordinates.getMinPageScaleFactor();
    }

    public float getMaxPageScaleFactor() {
        return mRenderCoordinates.getMaxPageScaleFactor();
    }

    public float getDeviceScaleFactor() {
        return mRenderCoordinates.getDeviceScaleFactor();
    }

    public float getWheelScrollFactor() {
        return mRenderCoordinates.getWheelScrollFactor();
    }

    public float fromLocalCssToPix(float css) {
        return mRenderCoordinates.fromLocalCssToPix(css);
    }

    public float getScrollX() {
        return mRenderCoordinates.getScrollX();
    }

    public float getScrollY() {
        return mRenderCoordinates.getScrollY();
    }

    public float getScrollXPix() {
        return mRenderCoordinates.getScrollXPix();
    }

    public float getScrollYPix() {
        return mRenderCoordinates.getScrollYPix();
    }

    public int getScrollXPixInt() {
        return mRenderCoordinates.getScrollXPixInt();
    }

    public int getScrollYPixInt() {
        return mRenderCoordinates.getScrollYPixInt();
    }

    public float getContentWidthCss() {
        return mRenderCoordinates.getContentWidthCss();
    }

    public float getContentHeightCss() {
        return mRenderCoordinates.getContentHeightCss();
    }

    public float getContentWidthPix() {
        return mRenderCoordinates.getContentWidthPix();
    }

    public float getContentHeightPix() {
        return mRenderCoordinates.getContentHeightPix();
    }

    public int getContentWidthPixInt() {
        return mRenderCoordinates.getContentWidthPixInt();
    }

    public int getContentHeightPixInt() {
        return mRenderCoordinates.getContentHeightPixInt();
    }

    public void setDeviceScaleFactor(float dipScale, Context context) {
        mRenderCoordinates.setDeviceScaleFactor(dipScale, context);
    }
}
