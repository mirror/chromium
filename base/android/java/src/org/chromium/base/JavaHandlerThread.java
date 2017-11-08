// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.lang.Thread.UncaughtExceptionHandler;

/**
 * Thread in Java with an Android Handler. This class is not thread safe.
 */
@JNINamespace("base::android")
public class JavaHandlerThread {
    private final HandlerThread mThread;

    private Throwable mUnhandledException;

    /**
     * Construct a java-only instance. Can be connected with native side later.
     * Useful for cases where a java thread is needed before native library is loaded.
     */
    public JavaHandlerThread(String name) {
        mThread = new HandlerThread(name);
    }

    @CalledByNative
    private static JavaHandlerThread create(String name) {
        return new JavaHandlerThread(name);
    }

    public Looper getLooper() {
        assert hasStarted();
        return mThread.getLooper();
    }

    public void maybeStart() {
        if (hasStarted()) return;
        mThread.start();
    }

    @CalledByNative
    private void startAndInitialize(final long nativeThread, final long nativeEvent) {
        maybeStart();
        new Handler(mThread.getLooper()).post(new Runnable() {
            @Override
            public void run() {
                nativeInitializeThread(nativeThread, nativeEvent);
            }
        });
    }

    @CalledByNative
    private void quitThreadSafely(final long nativeThread) {
        // Allow pending java tasks to run, but don't run any delayed or newly queued up tasks.
        new Handler(mThread.getLooper()).post(new Runnable() {
            @Override
            public void run() {
                mThread.quit();
                nativeOnLooperStopped(nativeThread);
            }
        });
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            // When we can, signal that new tasks queued up won't be run.
            mThread.getLooper().quitSafely();
        }
    }

    @CalledByNative
    private void joinThread() {
        boolean joined = false;
        while (!joined) {
            try {
                mThread.join();
                joined = true;
            } catch (InterruptedException e) {
            }
        }
    }

    private boolean hasStarted() {
        return mThread.getState() != Thread.State.NEW;
    }

    @CalledByNative
    private boolean isAlive() {
        return mThread.isAlive();
    }

    // TODO(mthiesse): Should this thread always listen for uncaught exceptions and invoke thread
    // cleanup when an uncaught java exception crashes the thread? It should be safe to call
    // OnLooperStopped from the UncaughtExceptionHandler.
    @CalledByNative
    private void listenForUncaughtExceptions() {
        mThread.setUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                mUnhandledException = e;
            }
        });
    }

    @CalledByNative
    private Throwable getUncaughtExceptionIfAny() {
        return mUnhandledException;
    }

    private native void nativeInitializeThread(long nativeJavaHandlerThread, long nativeEvent);
    private native void nativeOnLooperStopped(long nativeJavaHandlerThread);
}
