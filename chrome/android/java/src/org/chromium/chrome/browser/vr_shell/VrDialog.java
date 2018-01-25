// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.graphics.PointF;
import android.os.Handler;
import android.os.Message;
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
    private static final int MAX_NUM_POINTERS = 16;
    private PointerProperties[] mPointerProperties;
    private PointerCoords[] mPointerCoords;
    private long mDownTimeInMs;
    private boolean mIsDown = false;

    private FrameLayout mVrLayout;
    private Handler mHandler;

    /**
     * The default constructor of VrDialog. Starts a new {@link Handler} on
     * UI Thread that will be used to send the events to the Dialog.
     */
    public VrDialog() {
        mPointerProperties = new PointerProperties[MAX_NUM_POINTERS];
        mPointerCoords = new PointerCoords[MAX_NUM_POINTERS];

        mHandler = new Handler(ThreadUtils.getUiThreadLooper()) {
            private FrameLayout mLayout;
            @Override
            public void handleMessage(Message inputMessage) {
                PointF point = (PointF) inputMessage.obj;
                setPointer(0 /* index */, (int) point.x, (int) point.y, 0 /* id */,
                        MotionEvent.TOOL_TYPE_FINGER);
                long eventTime = SystemClock.uptimeMillis();
                inject(inputMessage.what, 1 /* pointerCount */, eventTime);
            }
        };
    }

    /**
     * @return The dialog that is set to be shown in VR.
     */
    public FrameLayout getLayout() {
        return mVrLayout;
    }

    /**
     * Set the layout to be shown in VR.
     */
    public void setLayout(FrameLayout vrLayout) {
        mVrLayout = vrLayout;
    }

    /**
     * Dismiss whatever dialog that is shown in VR.
     */
    public void dismiss() {
        VrShellDelegate.closeVrDialog();
    }

    /**
     * Initialize a dialog in VR based on the layout that was set by {@link
     * #setLayout(FrameLayout)}. This also adds a OnLayoutChangeListener to make sure that Dialog in
     * VR has the correct size.
     */
    public void initVrDialog() {
        mVrLayout.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                VrShellDelegate.setDialogSize(mVrLayout.getWidth(), mVrLayout.getHeight());
            }
        });
        long nativeVrDialog = nativeInitVrDialog(mVrLayout.getWidth(), mVrLayout.getHeight());
        VrShellDelegate.initVrDialog(nativeVrDialog, mVrLayout.getWidth(), mVrLayout.getHeight());
    }

    @CalledByNative
    public void onMouseMove(float x, float y) {
        if (!mIsDown) return;
        Message message = mHandler.obtainMessage(MotionEvent.ACTION_MOVE,
                new PointF(x * getLayout().getWidth(), y * getLayout().getHeight()));
        mHandler.sendMessage(message);
    }

    @CalledByNative
    public void onMouseDown(float x, float y) {
        mIsDown = true;
        Message message = mHandler.obtainMessage(MotionEvent.ACTION_DOWN,
                new PointF(x * getLayout().getWidth(), y * getLayout().getHeight()));
        mHandler.sendMessage(message);
    }

    @CalledByNative
    public void onMouseUp(float x, float y) {
        mIsDown = false;
        Message message = mHandler.obtainMessage(MotionEvent.ACTION_UP, new PointF(0, 0));
        mHandler.sendMessage(message);
    }

    private void setPointer(int index, int x, int y, int id, int toolType) {
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
            case MotionEvent.ACTION_MOVE: {
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs, action,
                        pointerCount, mPointerProperties, mPointerCoords, 0, 0, 1, 1, 0, 0,
                        InputDevice.SOURCE_TOUCHSCREEN, 0);
                getLayout().dispatchTouchEvent(event);
                event.recycle();
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

    private native long nativeInitVrDialog(int width, int height);
}
