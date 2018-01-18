// Copyright 2018 The Chromium Authors. All rights reserved.
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
import android.widget.FrameLayout;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Manages the communication with native VR implementation as well as
 * input routing to the views that are showin in VR.
 */
@JNINamespace("vr_shell")
public class VrDialog {
    private Matrix mInvMatrix = new Matrix();
    private float[] mPoint = new float[2];

    private long mNativeVrDialog;
    private boolean mIsDown = false;
    private boolean mIsShowing = false;
    protected DialogInterface.OnDismissListener mOnDismissListener;

    private PointerProperties[] mPointerProperties;
    private PointerCoords[] mPointerCoords;
    private long mDownTimeInMs;
    private FrameLayout mVrLayout;
    protected static final int MAX_NUM_POINTERS = 16;

    public VrDialog() {
        mPointerProperties = new PointerProperties[MAX_NUM_POINTERS];
        mPointerCoords = new PointerCoords[MAX_NUM_POINTERS];
    }

    public FrameLayout getLayout() {
        return mVrLayout;
    }
    public void setLayout(FrameLayout vrLayout) {
        mVrLayout = vrLayout;
    }

    public void dismiss() {
        closeVrDialog();
    }

    public void setOnDismissListener(DialogInterface.OnDismissListener onDismissListener) {
        mOnDismissListener = onDismissListener;
    }

    public boolean show() {
        mIsShowing = true;
        return false;
    }

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

    protected void closeVrDialog() {
        VrShellDelegate.closeVrDialog();
    }

    public void initVrDialog() {
        mVrLayout.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                VrShellDelegate.setDialogSize(mVrLayout.getWidth(), mVrLayout.getHeight());
            }
        });
        mNativeVrDialog = nativeInitVrDialog(mVrLayout.getWidth(), mVrLayout.getHeight());
        VrShellDelegate.initVrDialog(mNativeVrDialog, mVrLayout.getWidth(), mVrLayout.getHeight());
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

    /**
     * Set the parent dialog. This will be used to close th dialog.
     */
    public void setParent(Dialog dialog) {}

    /**
     * Set additional buttons that are not included in the main view
     */
    public void setButton(
            int whichButton, CharSequence text, DialogInterface.OnClickListener listener) {}

    /**
     * Sets the main view of the dialog.
     */
    public void setView(View view) {}

    private native long nativeInitVrDialog(int width, int height);
}
