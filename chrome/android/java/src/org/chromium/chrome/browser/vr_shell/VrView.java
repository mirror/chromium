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
    private long mDownTimeInMs;
    private LinearLayout mVrLayout;
    protected static final int MAX_NUM_POINTERS = 16;

    public VrView(long vrShellDelegate) {
        mNativeVrShellDelegate = vrShellDelegate;
        mPointerProperties = new PointerProperties[MAX_NUM_POINTERS];
        mPointerCoords = new PointerCoords[MAX_NUM_POINTERS];
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
                setPointer(0 /* index */, (int) (x * getLayout().getWidth()),
                        (int) (y * getLayout().getHeight()), 0 /* id */,
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
                setPointer(0 /* index */, (int) (x * getLayout().getWidth()),
                        (int) (y * getLayout().getHeight()), 0 /* id */,
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
                setPointer(0 /* index */, 0, 0, 0 /* id */, MotionEvent.TOOL_TYPE_FINGER);

                long eventTime = SystemClock.uptimeMillis();
                inject(MotionEvent.ACTION_UP, 1 /* pointerCount */, eventTime);
            }
        });
    }

    protected void closeVrView() {
        nativeCloseVrView(mNativeVrShellDelegate);
    }

    protected void initVrView() {
        mVrLayout.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                nativeSetSize(mNativeVrShellDelegate, mVrLayout.getWidth(), mVrLayout.getHeight());
            }
        });
        nativeInitVrView(mNativeVrShellDelegate, mVrLayout.getWidth(), mVrLayout.getHeight());
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
        switch (action) {
            case MotionEvent.ACTION_DOWN: {
                mDownTimeInMs = timeInMs;
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs,
                        MotionEvent.ACTION_DOWN, 1, mPointerProperties, mPointerCoords, 0, 0, 1, 1,
                        0, 0, InputDevice.SOURCE_TOUCHSCREEN, 0);
                getLayout().dispatchTouchEvent(event);
                event.recycle();

                break;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_MOVE:
            case MotionEvent.ACTION_CANCEL: {
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs, action,
                        pointerCount, mPointerProperties, mPointerCoords, 0, 0, 1, 1, 0, 0,
                        InputDevice.SOURCE_TOUCHSCREEN, 0);
                getLayout().dispatchTouchEvent(event);
                event.recycle();
                break;
            }
            case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_BUTTON_RELEASE: {
                MotionEvent eventRelease =
                        MotionEvent.obtain(mDownTimeInMs, timeInMs, action, 1, mPointerProperties,
                                mPointerCoords, 0, MotionEvent.BUTTON_PRIMARY, 1, 1, 0, 0, 0, 0);
                getLayout().dispatchGenericMotionEvent(eventRelease);
                eventRelease.recycle();
                break;
            }
            case MotionEvent.ACTION_SCROLL: {
                assert pointerCount == 1;
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs,
                        MotionEvent.ACTION_SCROLL, pointerCount, mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, InputDevice.SOURCE_CLASS_POINTER, 0);
                getLayout().dispatchGenericMotionEvent(event);
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
        getLayout().dispatchGenericMotionEvent(event);
        event.recycle();
    }

    private native long nativeInitVrView(long vrShellDelegate, int width, int height);
    private native void nativeSetSize(long vrShellDelegate, int width, int height);
    private native void nativeCloseVrView(long vrShellDelegate);
}
