// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.annotation.SuppressLint;
import android.content.Context;
import android.hardware.input.InputManager;
import android.hardware.input.InputManager.InputDeviceListener;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;

/**
 * A singleton that helps detecting changes in input devices through the interface
 * {@link InputDeviceObserver}.
 */
@JNINamespace("content")
public class InputDeviceObserver {
    private InputManager mInputManager;
    private InputDeviceListener mInputDeviceListener;
    private int mAttachedToWindowCounter;

    private InputDeviceObserver() {
        mInputDeviceListener = new InputDeviceListener() {
            // Override InputDeviceListener methods
            @Override
            public void onInputDeviceChanged(int deviceId) {
                nativeInputConfigurationChanged();
            }

            @Override
            public void onInputDeviceRemoved(int deviceId) {
                nativeInputConfigurationChanged();
            }

            @Override
            public void onInputDeviceAdded(int deviceId) {
                nativeInputConfigurationChanged();
            }
        };
    }

    /**
     * Notifies the InputDeviceObserver that a {@link ContentView} is attached to a window and it
     * should prepare itself for listening input changes.
     */
    public static void onAttachedToWindow(Context context) {
        assert ThreadUtils.runningOnUiThread();
        getInstance().attachedToWindow(context);
    }

    private void attachedToWindow(Context context) {
        if (mAttachedToWindowCounter++ == 0) {
            mInputManager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
            // Register an input device listener.
            mInputManager.registerInputDeviceListener(mInputDeviceListener, null);
        }
    }

    /**
     * Notifies the InputDeviceObserver that a {@link ContentView} is detached from it's window.
     */
    @SuppressLint("MissingSuperCall")
    public static void onDetachedFromWindow() {
        assert ThreadUtils.runningOnUiThread();
        getInstance().detachedFromWindow();
    }

    private void detachedFromWindow() {
        if (--mAttachedToWindowCounter == 0) {
            mInputManager.unregisterInputDeviceListener(mInputDeviceListener);
            mInputManager = null;
        }
    }

    // ------------------------------------------------------------

    private static InputDeviceObserver getInstance() {
        return LazyHolder.INSTANCE;
    }

    private native void nativeInputConfigurationChanged();

    private static class LazyHolder {
        private static final InputDeviceObserver INSTANCE = new InputDeviceObserver();
    }
}