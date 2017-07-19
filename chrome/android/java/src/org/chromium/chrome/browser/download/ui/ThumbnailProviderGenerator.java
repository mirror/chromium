// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.support.annotation.Nullable;

import org.chromium.base.annotations.CalledByNative;

/**
 * This class generates thumbnails for a given filepath by calling the native
 * ThumbnailProviderGenerator, which is owned and destroyed by the Java class.
 */
public class ThumbnailProviderGenerator implements ThumbnailProviderImplDelegate {
    /**
     * The native side pointer that is owned and destroyed by the Java class. It is set to -1 if it
     * has been destroyed.
     */
    private long mNativeThumbnailProviderGenerator;
    private ThumbnailProvider.ThumbnailRequest mRequest;
    private ThumbnailProviderGeneratorObserver mObserver;

    public ThumbnailProviderGenerator() {}

    private long getNativeThumbnailGenerator() {
        if (mNativeThumbnailProviderGenerator == 0) {
            mNativeThumbnailProviderGenerator = nativeInit();
        }
        return mNativeThumbnailProviderGenerator;
    }

    /** Asynchronously make thumbnail */
    @Override
    public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request,
            ThumbnailProviderGeneratorObserver observer) {
        if (destroyed()) return;

        mRequest = request;
        mObserver = observer;
        nativeRetrieveThumbnail(
                getNativeThumbnailGenerator(), mRequest.getFilePath(), mRequest.getIconSize());
    }

    private boolean destroyed() {
        return mNativeThumbnailProviderGenerator == -1;
    }

    @Override
    public void destroy() {
        nativeDestroy(mNativeThumbnailProviderGenerator);
        mNativeThumbnailProviderGenerator = -1;
    }

    @CalledByNative
    private void onThumbnailRetrieved(String filePath, @Nullable Bitmap bitmap) {
        mObserver.onThumbnailRetrieved(filePath, bitmap);
    }

    private native long nativeInit();
    private native void nativeDestroy(long nativeThumbnailProviderGenerator);
    private native void nativeRetrieveThumbnail(
            long nativeThumbnailProviderGenerator, String filePath, int thumbnailSize);
}