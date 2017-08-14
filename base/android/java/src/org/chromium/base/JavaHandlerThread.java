// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.MessageQueue;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Thread in Java with an Android Handler. This class is not thread safe.
 */
@JNINamespace("base::android")
public class JavaHandlerThread {
    private final HandlerThread mThread;

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

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
    @CalledByNative
    private void stop(final long nativeThread) {
        assert hasStarted();
        new Handler(mThread.getLooper()).post(new Runnable() {
            @Override
            public void run() {
                nativeStopThread(nativeThread);
                MessageQueue queue = mThread.getLooper().getQueue();
                // We add an idle handler so that we can run the thread cleanup code after the run
                // loop has detected an idle state and quit properly.
                queue.addIdleHandler(new MessageQueue.IdleHandler() {
                    @Override
                    public boolean queueIdle() {
                        // The queue is empty, so all tasks including delayed tasks have been
                        // delivered. This is equivalent to calling quitSafely here.
                        mThread.getLooper().quit();
                        nativeOnLooperStopped(nativeThread);
                        return false;
                    }
                });
            }
        });
        try {
            mThread.join();
        } catch (InterruptedException e) {
        }
    }

    private boolean hasStarted() {
        return mThread.getState() != Thread.State.NEW;
    }

    private native void nativeInitializeThread(long nativeJavaHandlerThread, long nativeEvent);
    private native void nativeStopThread(long nativeJavaHandlerThread);
    private native void nativeOnLooperStopped(long nativeJavaHandlerThread);
}
