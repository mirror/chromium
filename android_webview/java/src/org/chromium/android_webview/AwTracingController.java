// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.support.annotation.Nullable;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Manages tracing functionality in WebView.
 */
@JNINamespace("android_webview")
public class AwTracingController {
    private static final String TAG = "cr.AwTracingController";

    private AwTracingOutputStream mOutputStream;

    // TODO: consider caching a mIsTracing value for efficiency.
    // boolean mIsTracing;

    /**
     * Interface for the callbacks to return tracing data.
     */
    public interface AwTracingOutputStream {
        public void write(byte[] chunk);
        public void complete();
    }

    /**
     * Holds tracing configuration: categories and tracing mode.
     */
    public static class AwTracingConfig {
        private String mCategoryFilter;
        private int mTracingMode;

        public String getCategoryFilter() {
            return mCategoryFilter;
        }
        public int getTracingMode() {
            return mTracingMode;
        }
    }

    public AwTracingController() {
        mNativeAwTracingController = nativeInit();
    }

    // Start tracing.
    public boolean start(AwTracingConfig tracingConfig) {
        if (isTracing()) {
            return false;
        }

        boolean result = nativeStart(mNativeAwTracingController, tracingConfig.getCategoryFilter(),
                tracingConfig.getTracingMode());
        return result;
    }

    // Stop tracing and flush tracing data.
    public boolean stopAndFlush(@Nullable AwTracingOutputStream outputStream) {
        if (!isTracing()) return false;
        mOutputStream = outputStream;
        nativeStopAndFlush(mNativeAwTracingController);
        return true;
    }

    public boolean isTracing() {
        // or just use the native TracingController::isTracing()
        return nativeIsTracing(mNativeAwTracingController);
    }

    @CalledByNative
    private void onTraceDataChunkReceived(byte[] data) {
        if (mOutputStream != null) {
            mOutputStream.write(data);
        }
    }

    @CalledByNative
    private void onTraceDataComplete() {
        if (mOutputStream != null) {
            mOutputStream.complete();
        }
    }

    private long mNativeAwTracingController;
    private native long nativeInit();
    private native boolean nativeStart(
            long nativeAwTracingController, String categories, int traceMode);
    private native boolean nativeStopAndFlush(long nativeAwTracingController);
    private native boolean nativeIsTracing(long nativeAwTracingController);
}
