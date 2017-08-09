// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.support.annotation.Nullable;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;

/**
 * This class generates thumbnails for a given filepath by calling the native
 * ThumbnailProviderGenerator, which is owned and destroyed by the Java class. Multiple
 * {@link ThumbnailProvider.ThumbnailRequest} may be processed at a time.
 *
 * After {@link ThumbnailGenerator#destroy()}, assume that this class will not be called again.
 */
public class ThumbnailGenerator {
    // The native side pointer that is owned and destroyed by the Java class.
    private long mNativeThumbnailGenerator;

    private long getNativeThumbnailGenerator() {
        if (mNativeThumbnailGenerator == 0) {
            mNativeThumbnailGenerator = nativeInit();
        }
        return mNativeThumbnailGenerator;
    }

    /**
     * Asynchronously generates the requested thumbnail.
     * @param request The request for a thumbnail.
     * @param callback The class to call back to after thumbnail has been generated.
     */
    public void retrieveThumbnail(
            ThumbnailProvider.ThumbnailRequest request, ThumbnailGeneratorCallback callback) {
        ThreadUtils.assertOnUiThread();
        nativeRetrieveThumbnail(getNativeThumbnailGenerator(), request.getContentId(),
                request.getFilePath(), request.getIconSize(), callback);
    }

    /**
     * Destroys the native {@link ThumbnailGenerator}.
     */
    public void destroy() {
        ThreadUtils.assertOnUiThread();
        nativeDestroy(mNativeThumbnailGenerator);
    }

    /**
     * Called when thumbnail has been generated.
     * @param contentId Content ID of the requested thumbnail.
     * @param requestedIconSize Width (pixel) of the requested thumbnail.
     * @param bitmap The requested thumbnail.
     * @param callback The class to call back to after thumbnail has been generated.
     */
    @CalledByNative
    @VisibleForTesting
    void onThumbnailRetrieved(String contentId, int requestedIconSize, @Nullable Bitmap bitmap,
            ThumbnailGeneratorCallback callback) {
        if (bitmap != null) {
            // The bitmap returned here is retrieved from the native side. The image decoder there
            // scales down the image (if it is too big) so that one of its sides is smaller than or
            // equal to the required size. We check here that the returned image satisfies this
            // criteria.
            assert Math.min(bitmap.getWidth(), bitmap.getHeight()) <= requestedIconSize;
        }

        callback.onThumbnailRetrieved(contentId, bitmap, requestedIconSize);
    }

    private native long nativeInit();
    private native void nativeDestroy(long nativeThumbnailGenerator);
    private native void nativeRetrieveThumbnail(long nativeThumbnailGenerator, String contentId,
            String filePath, int thumbnailSize, ThumbnailGeneratorCallback callback);
}