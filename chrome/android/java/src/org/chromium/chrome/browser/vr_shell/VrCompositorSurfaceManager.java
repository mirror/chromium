// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.graphics.drawable.Drawable;
import android.view.Surface;

import org.chromium.chrome.browser.compositor.CompositorSurfaceManager;

/**
 * Abstracts away the VrClassesWrapperImpl class, which may or may not be present at runtime
 * depending on compile flags.
 */
public class VrCompositorSurfaceManager implements CompositorSurfaceManager {
    private boolean mSurfaceRequestPending;
    private boolean mSurfaceProvided;
    private Surface mSurface;
    private int mFormat;
    private int mWidth;
    private int mHeight;

    // Client that we notify about surface change events.
    private SurfaceManagerCallbackTarget mClient;

    public VrCompositorSurfaceManager(SurfaceManagerCallbackTarget client) {
        mClient = client;
    }

    /* package */ void setSurface(Surface surface, int format, int width, int height) {
        mSurface = surface;
        mFormat = format;
        mWidth = width;
        mHeight = height;
        if (mSurfaceRequestPending) {
            mClient.surfaceCreated(mSurface);
            mClient.surfaceChanged(mSurface, mFormat, mWidth, mHeight);
            mSurfaceProvided = true;
        }
        mSurfaceRequestPending = false;
    }

    /* package */ void surfaceResized(int width, int height) {
        mWidth = width;
        mHeight = height;
        if (mSurfaceProvided) mClient.surfaceChanged(mSurface, mFormat, mWidth, mHeight);
    }

    /* package */ void surfaceDestroyed() {
        shutDown();
    }

    @Override
    public void shutDown() {
        if (mSurface != null && mSurfaceProvided) mClient.surfaceDestroyed();
        mSurface = null;
        mSurfaceProvided = false;
    }

    @Override
    public void requestSurface(int format) {
        if (mSurface == null) {
            mSurfaceRequestPending = true;
            return;
        }
        mClient.surfaceCreated(mSurface);
        mClient.surfaceChanged(mSurface, mFormat, mWidth, mHeight);
        mSurfaceProvided = true;
    }

    @Override
    public void doneWithUnownedSurface() {}

    @Override
    public void recreateSurfaceForJellyBean() {}

    @Override
    public void setBackgroundDrawable(Drawable background) {}

    @Override
    public void setWillNotDraw(boolean willNotDraw) {}

    @Override
    public void setVisibility(int visibility) {}
}
