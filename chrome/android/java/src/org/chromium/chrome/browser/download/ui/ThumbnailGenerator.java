// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.support.annotation.Nullable;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;

/**
 * This class generates thumbnails for a given filepath by calling the native
 * ThumbnailProviderGenerator, which is owned and destroyed by the Java class. Multiple
 * {@link ThumbnailProvider.ThumbnailRequest} may be processed at a time.
 *
 * This class requires the caller (ThumbnailDiskStorage) to call
 * {@link ThumbnailGenerator#setRequester(ThumbnailDiskStorage)} to create an instance.
 */
public class ThumbnailGenerator {
    /**
     * The native side pointer that is owned and destroyed by the Java class. It is set to -1 if it
     * has been destroyed.
     */
    private long mNativeThumbnailGenerator;
    private ThumbnailDiskStorage mStorage;

    public ThumbnailGenerator() {}

    /**
     * Sets the {@link ThumbnailDiskStorage} to call back to.
     * @param storage       The {@link ThumbnailDiskStorage} that is making requests.
     */
    public void setRequester(ThumbnailDiskStorage storage) {
        mStorage = storage;
    }

    private long getNativeThumbnailGenerator() {
        if (mNativeThumbnailGenerator == 0) {
            mNativeThumbnailGenerator = nativeInit();
        }
        return mNativeThumbnailGenerator;
    }

    /**
     * Asynchronously generates the requested thumbnail.
     * @param request       The request for a thumbnail.
     */
    public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request) {
        assert !isDestroyed();

        nativeRetrieveThumbnail(getNativeThumbnailGenerator(), request.getContentId(),
                request.getFilePath(), request.getIconSize(), request.shouldCacheToDisk());
    }

    private boolean isDestroyed() {
        return mNativeThumbnailGenerator == -1;
    }

    /**
     * Destroys the native {@link ThumbnailGenerator}.
     */
    public void destroy() {
        nativeDestroy(mNativeThumbnailGenerator);
        mNativeThumbnailGenerator = -1;
    }

    @CalledByNative
    @VisibleForTesting
    void onThumbnailRetrieved(
            String contentId, int requestedIconSize, @Nullable Bitmap bitmap, boolean shouldCache) {
        if (bitmap != null) {
            // The bitmap returned here is retrieved from the native side. The image decoder there
            // scales down the image (if it is too big) so that one of its sides is smaller than or
            // equal to the required size. We check here that the returned image satisfies this
            // criteria.
            assert Math.min(bitmap.getWidth(), bitmap.getHeight()) <= requestedIconSize;
        }

        mStorage.onThumbnailRetrieved(shouldCache, contentId, bitmap);
    }

    private native long nativeInit();
    private native void nativeDestroy(long nativeThumbnailGenerator);
    private native void nativeRetrieveThumbnail(long nativeThumbnailGenerator, String contentId,
            String filePath, int thumbnailSize, boolean shouldCache);
}