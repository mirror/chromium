// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.MotionEvent.PointerCoords;
import android.view.MotionEvent.PointerProperties;
import android.view.View;

import org.chromium.base.annotations.CalledByNative;

/**
 * Provides a Java-side implementation for injecting synthetic touch events.
 */
public class MotionEventSynthesizer {
    private static final int MAX_NUM_POINTERS = 16;

    private final View mTarget;
    private final PointerProperties[] mPointerProperties;
    private final PointerCoords[] mPointerCoords;
    private long mDownTimeInMs;

    @CalledByNative
    private static MotionEventSynthesizer create(View target) {
        return new MotionEventSynthesizer(target);
    }

    private MotionEventSynthesizer(View target) {
        mTarget = target;
        mPointerProperties = new PointerProperties[MAX_NUM_POINTERS];
        mPointerCoords = new PointerCoords[MAX_NUM_POINTERS];
    }

    @CalledByNative
    void setPointer(int index, int x, int y, int id) {
        assert (0 <= index && index < MAX_NUM_POINTERS);

        PointerCoords coords = new PointerCoords();
        coords.x = x;
        coords.y = y;
        coords.pressure = 1.0f;
        mPointerCoords[index] = coords;

        PointerProperties properties = new PointerProperties();
        properties.id = id;
        mPointerProperties[index] = properties;
    }

    @CalledByNative
    void setScrollDeltas(int x, int y, int dx, int dy) {
        setPointer(0, x, y, 0);
        mPointerCoords[0].setAxisValue(MotionEvent.AXIS_HSCROLL, dx);
        mPointerCoords[0].setAxisValue(MotionEvent.AXIS_VSCROLL, dy);
    }

    @CalledByNative
    void inject(int action, int pointerCount, long timeInMs) {
        switch (action) {
            case MotionEventAction.START: {
                mDownTimeInMs = timeInMs;
                MotionEvent event = MotionEvent.obtain(
                        mDownTimeInMs, timeInMs, MotionEvent.ACTION_DOWN, 1,
                        mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, 0, 0);
                mTarget.dispatchTouchEvent(event);
                event.recycle();

                if (pointerCount > 1) {
                    event = MotionEvent.obtain(
                            mDownTimeInMs, timeInMs,
                            MotionEvent.ACTION_POINTER_DOWN, pointerCount,
                            mPointerProperties, mPointerCoords,
                            0, 0, 1, 1, 0, 0, 0, 0);
                    mTarget.dispatchTouchEvent(event);
                    event.recycle();
                }
                break;
            }
            case MotionEventAction.MOVE: {
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs,
                        MotionEvent.ACTION_MOVE,
                        pointerCount, mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, 0, 0);
                mTarget.dispatchTouchEvent(event);
                event.recycle();
                break;
            }
            case MotionEventAction.CANCEL: {
                MotionEvent event = MotionEvent.obtain(
                        mDownTimeInMs, timeInMs, MotionEvent.ACTION_CANCEL, 1,
                        mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, 0, 0);
                mTarget.dispatchTouchEvent(event);
                event.recycle();
                break;
            }
            case MotionEventAction.END: {
                if (pointerCount > 1) {
                    MotionEvent event = MotionEvent.obtain(
                            mDownTimeInMs, timeInMs, MotionEvent.ACTION_POINTER_UP,
                            pointerCount, mPointerProperties, mPointerCoords,
                            0, 0, 1, 1, 0, 0, 0, 0);
                    mTarget.dispatchTouchEvent(event);
                    event.recycle();
                }

                MotionEvent event = MotionEvent.obtain(
                        mDownTimeInMs, timeInMs, MotionEvent.ACTION_UP, 1,
                        mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, 0, 0);
                mTarget.dispatchTouchEvent(event);
                event.recycle();
                break;
            }
            case MotionEventAction.SCROLL: {
                assert pointerCount == 1;
                MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs,
                        MotionEvent.ACTION_SCROLL, pointerCount, mPointerProperties, mPointerCoords,
                        0, 0, 1, 1, 0, 0, InputDevice.SOURCE_CLASS_POINTER, 0);
                mTarget.dispatchGenericMotionEvent(event);
                event.recycle();
                break;
            }
            case MotionEventAction.HOVER_ENTER:
            case MotionEventAction.HOVER_EXIT:
            case MotionEventAction.HOVER_MOVE: {
                injectHover(action, pointerCount, timeInMs);
                break;
            }
            default: {
                assert false : "Unreached";
                break;
            }
        }
    }

    private void injectHover(int action, int pointerCount, long timeInMs) {
        assert pointerCount == 1;
        int androidAction = MotionEvent.ACTION_HOVER_ENTER;
        if (MotionEventAction.HOVER_EXIT == action) androidAction = MotionEvent.ACTION_HOVER_EXIT;
        if (MotionEventAction.HOVER_MOVE == action) androidAction = MotionEvent.ACTION_HOVER_MOVE;
        MotionEvent event = MotionEvent.obtain(mDownTimeInMs, timeInMs, androidAction, pointerCount,
                mPointerProperties, mPointerCoords, 0, 0, 1, 1, 0, 0,
                InputDevice.SOURCE_CLASS_POINTER, 0);
        mTarget.dispatchGenericMotionEvent(event);
        event.recycle();
    }
}
