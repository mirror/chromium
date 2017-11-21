// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.Matrix;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.MotionEvent.PointerCoords;
import android.view.MotionEvent.PointerProperties;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.DialogRenderer;

@JNINamespace("vr_shell")
public class VrView implements DialogRenderer {
    private Matrix mInvMatrix = new Matrix();
    private float[] mPoint = new float[2];

    private long mNativeVrShellDelegate;
    private long mNativeVrView;
    private boolean mIsDown = false;
    private boolean mIsShowing = false;

    private PointerProperties[] mPointerProperties;
    private PointerCoords[] mPointerCoords;
    private View mTarget;
    private long mDownTimeInMs;
    private LinearLayout mVrLayout;
    protected static final int MAX_NUM_POINTERS = 16;

    public VrView(long vrShellDelegate) {
        mNativeVrShellDelegate = vrShellDelegate;
        mPointerProperties = new PointerProperties[MAX_NUM_POINTERS];
        mPointerCoords = new PointerCoords[MAX_NUM_POINTERS];
    }
    public View findChildByPosition(ViewGroup parent, ViewGroup current, float x, float y) {
        int count = current.getChildCount();
        for (int i = count - 1; i >= 0; i--) {
            View child = current.getChildAt(i);
            if (child instanceof ViewGroup) {
                View result = findChildByPosition(
                        current, (ViewGroup) child, x - current.getX(), y - current.getY());
                if (result != null) return result;
            }
            if (child.getVisibility() == View.VISIBLE && child.isClickable() && child.isEnabled()) {
                if (isPositionInChildView(current, child, x - current.getX(), y - current.getY())) {
                    return child;
                }
            }
        }
        return null;
    }

    private boolean isPositionInChildView(ViewGroup parent, View child, float x, float y) {
        mPoint[0] = x + parent.getScrollX() - child.getLeft();
        mPoint[1] = y + parent.getScrollY() - child.getTop();
        Matrix childMatrix = child.getMatrix();
        if (!childMatrix.isIdentity()) {
            childMatrix.invert(mInvMatrix);
            mInvMatrix.mapPoints(mPoint);
        }
        x = mPoint[0];
        y = mPoint[1];
        return x >= 0 && y >= 0 && x < child.getWidth() && y < child.getHeight();
    }

    protected LinearLayout getLayout() {
        return mVrLayout;
    }
    protected void setLayout(LinearLayout vrLayout) {
        mVrLayout = vrLayout;
    }

    @Override
    public void setParent(Dialog dialog) {}

    @Override
    public void dismiss() {
        closeVrView();
    }

    @Override
    public void setView(View view) {}

    @Override
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener) {}

    @Override
    public boolean show() {
        mIsShowing = true;
        return false;
    }

    @Override
    public boolean isShowing() {
        return mIsShowing;
    }

    @CalledByNative
    public void onMouseMove(float x, float y) {
        if (!mIsDown) return;
        ThreadUtils.runOnUiThread(new Runnable() {
            public void run() {
                setPointer(0 /* index */, (int) (x * 1200), (int) (y * 800), 0 /* id */,
                        MotionEvent.TOOL_TYPE_FINGER);
                long eventTime = SystemClock.uptimeMillis();
                inject(MotionEvent.ACTION_MOVE, 1 /* pointerCount */, eventTime);
            }
        });
    }

    @CalledByNative
    public void onMouseDown(float x, float y) {
        mIsDown = true;
        ThreadUtils.runOnUiThread(new Runnable() {
            public void run() {
                setPointer(0 /* index */, (int) (x * 1200), (int) (y * 800), 0 /* id */,
                        MotionEvent.TOOL_TYPE_FINGER);
                long eventTime = SystemClock.uptimeMillis();
                inject(MotionEvent.ACTION_DOWN, 1 /* pointerCount */, eventTime);
            }
        });
    }
    @CalledByNative
    public void onMouseUp(float x, float y) {
        mIsDown = false;
        ThreadUtils.runOnUiThread(new Runnable() {
            public void run() {
                setPointer(0 /* index */, (int) (x * 1200), (int) (y * 800), 0 /* id */,
                        MotionEvent.TOOL_TYPE_FINGER);
                long eventTime = SystemClock.uptimeMillis();
                inject(MotionEvent.ACTION_UP, 1 /* pointerCount */, eventTime);

                /*                View target = findChildByPosition(
                                        mVrLayout, mVrLayout, (int) (x * 1200), (int) (y * 800));
                                if (target != null) {
                                   target.performClick();
                                }*/
            }
        });
    }

    protected void closeVrView() {
        nativeCloseVrView(mNativeVrShellDelegate);
    }

    protected void initVrView() {
        mTarget = mVrLayout;
        nativeInitVrView(mNativeVrShellDelegate);
    }

    public void setPointer(int index, int x, int y, int id, int toolType) {
        assert(0 <= index && index < MAX_NUM_POINTERS);

        PointerCoords coords = new PointerCoords();
        coords.x = x;
        coords.y = y;
        coords.pressure = 1.0f;
        mPointerCoords[index] = coords;

        PointerProperties properties = new PointerProperties();
        properties.id = id;
        properties.toolType = toolType;
        mPointerProperties[index] = properties;
    }

    private void inject(int action, int pointerCount, long timeInMs) {
        // TODO enable this and fix events.
        // if (true) return;
        switch (action) {
            case MotionEvent.ACTION_DOWN: {
                mDownTimeInMs = timeInMs;
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs,
                        MotionEvent.ACTION_DOWN, 1, mPointerProperties, mPointerCoords, 0, 0, 1, 1,
                        0, 0, InputDevice.SOURCE_TOUCHSCREEN, 0);
                mTarget.dispatchTouchEvent(event);
                event.recycle();

                break;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_MOVE:
            case MotionEvent.ACTION_CANCEL: {
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs, action,
                        pointerCount, mPointerProperties, mPointerCoords, 0, 0, 1, 1, 0, 0,
                        InputDevice.SOURCE_TOUCHSCREEN, 0);
                mTarget.dispatchTouchEvent(event);
                event.recycle();
                break;
            }
            case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_BUTTON_RELEASE: {
                MotionEvent eventRelease =
                        MotionEvent.obtain(mDownTimeInMs, timeInMs, action, 1, mPointerProperties,
                                mPointerCoords, 0, MotionEvent.BUTTON_PRIMARY, 1, 1, 0, 0, 0, 0);
                mTarget.dispatchGenericMotionEvent(eventRelease);
                eventRelease.recycle();
                break;
            }
            case MotionEvent.ACTION_SCROLL: {
                assert pointerCount == 1;
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs,
                        MotionEvent.ACTION_SCROLL, pointerCount, mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, InputDevice.SOURCE_CLASS_POINTER, 0);
                mTarget.dispatchGenericMotionEvent(event);
                event.recycle();
                break;
            }
            case MotionEvent.ACTION_HOVER_ENTER:
            case MotionEvent.ACTION_HOVER_EXIT:
            case MotionEvent.ACTION_HOVER_MOVE: {
                injectHover(action, pointerCount, timeInMs);
                break;
            }
            default: {
                assert false : "Unreached";
                break;
            }
        }
    }

    private void injectHover(int androidAction, int pointerCount, long timeInMs) {
        assert pointerCount == 1;
        MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs, androidAction, pointerCount,
                mPointerProperties, mPointerCoords, 0, 0, 1, 1, 0, 0,
                InputDevice.SOURCE_CLASS_POINTER, 0);
        mTarget.dispatchGenericMotionEvent(event);
        event.recycle();
    }

    private native long nativeInitVrView(long vrShellDelegate);
    private native void nativeCloseVrView(long vrShellDelegate);
}
