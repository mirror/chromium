// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

/** Provides thumbnails that represent different files. */
public interface ThumbnailProvider {
    /**
     * Used to request the retrieval of a thumbnail. Both content ID and file path are required to
     * be @NonNull.
     */
    public static interface ThumbnailRequest {
        /** Local storage path to the file. */
        @Nullable
        String getFilePath();

        /** Content ID that uniquely identifies the file. */
        @Nullable
        String getContentId();

        /** Called when a requested thumbnail is ready. */
        void onThumbnailRetrieved(@NonNull String identifier, @Nullable Bitmap thumbnail);

        /** The requested size (maximum dimension (pixel) of the smaller side) of the thumbnail to
         * be retrieved. */
        int getIconSize();

        /** The URL the file was downloaded from. */
        @NonNull
        String getUrl();

        /** The file type. */
        int getFileType();
    }

    /** Destroys the class. */
    void destroy();

    /**
     * Calls {@link ThumbnailRequest#onThumbnailRetrieved} immediately if the thumbnail is cached.
     * Otherwise, asynchronously fetches the thumbnail from the provider and calls
     * {@link ThumbnailRequest#onThumbnailRetrieved} when the result is ready.
     * @param request Parameters that describe the thumbnail being retrieved.
     */
    void getThumbnail(ThumbnailRequest request);

    /**
     * Removes the thumbnails (different sizes) with {@code contentId} from disk (if disk-cached).
     * @param contentId The content ID of the thumbnail to remove.
     * @param fileType The file type (image or page) the thumbnail is for.
     */
    void removeThumbnailsFromDisk(String contentId, int filterType);

    /** Removes a particular request from the pending queue. */
    void cancelRetrieval(ThumbnailRequest request);
}