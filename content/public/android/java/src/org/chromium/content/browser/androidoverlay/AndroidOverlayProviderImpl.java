// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.androidoverlay;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.media.mojom.AndroidOverlay;
import org.chromium.media.mojom.AndroidOverlayClient;
import org.chromium.media.mojom.AndroidOverlayConfig;
import org.chromium.media.mojom.AndroidOverlayProvider;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.services.service_manager.InterfaceFactory;

/**
 * Default impl of AndroidOverlayProvider.  Creates AndroidOverlayImpls.  We're a singleton, in the
 * sense that all provider clients talk to the same instance in the browser.
 */
@JNINamespace("content")
public class AndroidOverlayProviderImpl implements AndroidOverlayProvider {
    private static final String TAG = "AndroidOverlayProvider";

    // Maximum number of concurrent overlays that we allow.
    private static final int MAX_OVERLAYS = 1;

    // Handle to the AndroidOverlayProviderImpl singleton.
    private long mNativeHandle = 0;

    // We maintain a thread with a Looper for the AndroidOverlays to use, since Dialog requires one.
    // We don't want this to be the native thread that's used to create them (the browser UI thread)
    // since we don't want to block that waiting for sync callbacks from Android, such as
    // surfaceDestroyed.  Instead, we run all AndroidOverlays on one shared overlay-ui thread.
    private HandlerThread mOverlayUiThread;
    private Handler mHandler;

    // Number of AndroidOverlays that have been created but not released.
    private int mNumOverlays;

    // Runnable that notifies us that a client has been released.
    private Runnable mNotifyReleasedRunnable = new Runnable() {
        @Override
        public void run() {
            notifyReleased();
        }
    };

    private long getNativeHandle() {
        if (mNativeHandle == 0) {
            mNativeHandle = nativeGetNativeInstance();
            assert mNativeHandle != 0;
        }

        return mNativeHandle;
    }

    /**
     * Create an overlay matching |config| for |client|, and bind it to |request|.  Remember that
     * potentially many providers are created.
     */
    @Override
    public void createOverlay(InterfaceRequest<AndroidOverlay> request, AndroidOverlayClient client,
            AndroidOverlayConfig config) {
        ThreadUtils.assertOnUiThread();

        boolean validFrame = nativeIsFrameValidAndVisible(
                getNativeHandle(), config.routingToken.high, config.routingToken.low);

        // Limit the number of concurrent surfaces.
        // Also drop requests for dead frames, pending frames and hidden WebContents.
        if (mNumOverlays >= MAX_OVERLAYS || !validFrame) {
            client.onDestroyed();
            return;
        }

        startThreadIfNeeded();
        mNumOverlays++;

        DialogOverlayImpl impl =
                new DialogOverlayImpl(client, config, mHandler, mNotifyReleasedRunnable);
        DialogOverlayImpl.MANAGER.bind(impl, request);
    }

    /**
     * Make sure that mOverlayUiThread and mHandler are ready for use, if needed.
     */
    private void startThreadIfNeeded() {
        if (mOverlayUiThread != null) return;

        mOverlayUiThread = new HandlerThread("AndroidOverlayThread");
        mOverlayUiThread.start();
        mHandler = new Handler(mOverlayUiThread.getLooper());
    }

    /**
     * Called by AndroidOverlays when they no longer need the thread via |mNotifyReleasedRunnable|.
     */
    private void notifyReleased() {
        ThreadUtils.assertOnUiThread();
        assert mNumOverlays > 0;
        mNumOverlays--;

        // We don't stop the looper thread here, else android can get mad when it tries to send
        // a message from the dialog on this thread.  AndroidOverlay might have to notify us
        // separately to tell us when it's done with the thread, if we don't want to wait until
        // then to start creating a new SV.
        // Instead, we just avoid shutting down the thread at all for now.
    }

    // Remember that we can't tell which client disconnected.
    @Override
    public void close() {}

    // Remember that we can't tell which client disconnected.
    @Override
    public void onConnectionError(MojoException e) {}

    // Are overlays supported by the embedder?
    @CalledByNative
    private static boolean areOverlaysSupported() {
        return true;
    }

    /**
     * Mojo factory.
     */
    public static class Factory implements InterfaceFactory<AndroidOverlayProvider> {
        private static AndroidOverlayProviderImpl sImpl;
        public Factory(Context context) {}

        @Override
        public AndroidOverlayProvider createImpl() {
            if (sImpl == null) sImpl = new AndroidOverlayProviderImpl();
            return sImpl;
        }
    }

    /**
     * Gets the native singleton instance.
     */
    private native long nativeGetNativeInstance();

    /**
     * Returns whether the RenderFrameHostImpl identified by the
     * OverlayRoutingToken is valid for Overlay purposes.
     * We define "valid" as an alive, current frame whose WebContents is shown.
     * I.e. it is valid if the creation of an AndroidOverlay in this frame would
     * lead to an immediate and constructive use of the overlay.
     */
    private native boolean nativeIsFrameValidAndVisible(
            long nativeAndroidOverlayProviderImpl, long tokenHigh, long tokenLow);
}
