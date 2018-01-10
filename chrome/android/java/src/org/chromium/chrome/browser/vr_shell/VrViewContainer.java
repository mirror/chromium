// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver.OnPreDrawListener;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import org.chromium.base.TraceEvent;

/**
 * Holds views that will be drawn into a texture when in VR. Has no effect outside of VR.
 */
public class VrViewContainer extends FrameLayout {
    boolean mInVr;
    private Surface mSurface;
    private OnPreDrawListener mPredrawListener;
    private Drawable mBackgroundToRestore;
    private ViewGroup.LayoutParams mLayoutParams;

    /**
     * See {@link FrameLayout#FrameLayout(Context)}.
     */
    public VrViewContainer(Context context) {
        super(context);
        mLayoutParams = getLayoutParams();
        mBackgroundToRestore = getBackground();
        // We need a pre-draw listener to invalidate the native page because scrolling usually
        // doesn't trigger an onDraw call, so our texture won't get updated.
        mPredrawListener = new OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                if (isDirty()) invalidate();
                return true;
            }
        };
    }

    // We want screen taps to only be routed to the GvrUiLayout, and not propagate through to the
    // java views. So screen taps are ignored here and received by the GvrUiLayout.
    // Touch events generated from the VR controller are injected directly into this View's
    // child.
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (mInVr) return false;
        return super.dispatchTouchEvent(event);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        if (!mInVr) {
            super.dispatchDraw(canvas);
            return;
        }
        if (mSurface == null) return;
        try (TraceEvent e = TraceEvent.scoped("VrViewContainer.dispatchDraw")) {
            // TODO(mthiesse): Support mSurface.lockHardwareCanvas(); https://crbug.com/692775
            final Canvas surfaceCanvas = mSurface.lockCanvas(null);
            surfaceCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
            super.dispatchDraw(surfaceCanvas);
            mSurface.unlockCanvasAndPost(surfaceCanvas);
        }
    }

    @Override
    public void setLayoutParams(ViewGroup.LayoutParams params) {
        mLayoutParams = params;
        if (mInVr) return;
        super.setLayoutParams(params);
    }

    /* package */ View getInputTarget() {
        return getChildAt(0);
    }

    /* package */ void setSurface(Surface surface) {
        mSurface = surface;
        invalidate();
    }

    /* package */ void resizeForVr(int width, int height) {
        super.setLayoutParams(new LinearLayout.LayoutParams(width, height));
    }

    /* package */ void onEnterVr() {
        getViewTreeObserver().addOnPreDrawListener(mPredrawListener);
        setBackgroundColor(Color.TRANSPARENT);
        mInVr = true;
    }

    /* package */ void onExitVr() {
        mInVr = false;
        getViewTreeObserver().removeOnPreDrawListener(mPredrawListener);
        setBackground(mBackgroundToRestore);
        setLayoutParams(mLayoutParams);
    }
}
