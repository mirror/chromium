// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.Nullable;
import android.support.v4.util.LruCache;
import android.text.TextUtils;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;

import java.lang.ref.WeakReference;
import java.util.ArrayDeque;
import java.util.Deque;

/**
 * Concrete implementation of {@link ThumbnailProvider}.
 *
 * Thumbnails are cached and shared across all ThumbnailProviderImpls.  The cache itself is LRU and
 * limited in size.  It is automatically garbage collected under memory pressure.
 *
 * A queue of requests is maintained in FIFO order.  Missing thumbnails are retrieved asynchronously
 * by the native ThumbnailProvider, which is owned and destroyed by the Java class.
 *
 * TODO(dfalcantara): Figure out how to send requests simultaneously to the utility process without
 *                    duplicating work to decode the same image for two different requests.
 */
public class ThumbnailProviderImpl implements ThumbnailProvider {
    /** 5 MB of thumbnails should be enough for everyone. */
    private static final int MAX_CACHE_BYTES = 5 * 1024 * 1024;

    /**
     *  Weakly referenced cache containing thumbnails that can be deleted under memory pressure.
     *  Key in the cache is the filepath and the height/width (these are equal) of the thumbnail.
     *  Value is the thumbnail bitmap and its byte size.
     * */
    private static WeakReference<LruCache<ThumbnailKey, ThumbnailValue>> sBitmapCache =
            new WeakReference<>(null);

    /** Enqueues requests. */
    private final Handler mHandler;

    /** Queue of files to retrieve thumbnails for. */
    private final Deque<ThumbnailRequest> mRequestQueue;

    /** The native side pointer that is owned and destroyed by the Java class. */
    private long mNativeThumbnailProvider;

    /** Request that is currently having its thumbnail retrieved. */
    private ThumbnailRequest mCurrentRequest;

    public ThumbnailProviderImpl() {
        mHandler = new Handler(Looper.getMainLooper());
        mRequestQueue = new ArrayDeque<>();
        mNativeThumbnailProvider = nativeInit();
    }

    @Override
    public void destroy() {
        ThreadUtils.assertOnUiThread();
        nativeDestroy(mNativeThumbnailProvider);
        mNativeThumbnailProvider = 0;
    }

    @Override
    public void getThumbnail(ThumbnailRequest request) {
        String filePath = request.getFilePath();
        if (TextUtils.isEmpty(filePath)) return;

        Bitmap cachedBitmap = getBitmapFromCache(new ThumbnailKey(filePath, request.getIconSize()));
        if (cachedBitmap != null) {
            request.onThumbnailRetrieved(filePath, cachedBitmap);
            return;
        }

        mRequestQueue.offer(request);
        processQueue();
    }

    /** Removes a particular file from the pending queue. */
    @Override
    public void cancelRetrieval(ThumbnailRequest request) {
        if (mRequestQueue.contains(request)) mRequestQueue.remove(request);
    }

    private void processQueue() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                processNextRequest();
            }
        });
    }

    private Bitmap getBitmapFromCache(ThumbnailKey thumbnailKey) {
        ThumbnailValue cachedThumbnail = getBitmapCache().get(thumbnailKey);
        if (cachedThumbnail == null) return null;
        Bitmap cachedBitmap = cachedThumbnail.getBitmap();

        if (cachedBitmap == null) return null;
        assert !cachedBitmap.isRecycled();
        return cachedBitmap;
    }

    private void processNextRequest() {
        if (!isInitialized() || mCurrentRequest != null || mRequestQueue.isEmpty()) return;

        mCurrentRequest = mRequestQueue.poll();
        String currentFilePath = mCurrentRequest.getFilePath();

        ThumbnailKey thumbnailKey =
                new ThumbnailKey(currentFilePath, mCurrentRequest.getIconSize());

        Bitmap cachedBitmap = getBitmapFromCache(thumbnailKey);
        if (cachedBitmap == null) {
            // Asynchronously process the file to make a thumbnail.
            nativeRetrieveThumbnail(
                    mNativeThumbnailProvider, currentFilePath, mCurrentRequest.getIconSize());
        } else {
            // Send back the already-processed file.
            onThumbnailRetrieved(currentFilePath, cachedBitmap);
        }
    }

    @CalledByNative
    private void onThumbnailRetrieved(String filePath, @Nullable Bitmap bitmap) {
        if (bitmap != null) {
            assert mCurrentRequest.getIconSize() == bitmap.getHeight();

            ThumbnailKey thumbnailKey = new ThumbnailKey(filePath, bitmap.getHeight());
            ThumbnailValue thumbnailValue = new ThumbnailValue(bitmap, bitmap.getByteCount());
            getBitmapCache().put(thumbnailKey, thumbnailValue);

            mCurrentRequest.onThumbnailRetrieved(filePath, bitmap);
        }

        mCurrentRequest = null;
        processQueue();
    }

    private boolean isInitialized() {
        return mNativeThumbnailProvider != 0;
    }

    private static LruCache<ThumbnailKey, ThumbnailValue> getBitmapCache() {
        ThreadUtils.assertOnUiThread();

        LruCache<ThumbnailKey, ThumbnailValue> cache =
                sBitmapCache == null ? null : sBitmapCache.get();
        if (cache != null) return cache;

        // Create a new weakly-referenced cache.
        cache = new LruCache<ThumbnailKey, ThumbnailValue>(MAX_CACHE_BYTES) {
            @Override
            protected int sizeOf(ThumbnailKey thumbnailKey, ThumbnailValue thumbnailValue) {
                return thumbnailValue == null ? 0 : thumbnailValue.getSize();
            }
        };
        sBitmapCache = new WeakReference<>(cache);
        return cache;
    }

    /**
     * Holds the key for the cached thumbnails. Consists of the filepath to the thumbnail and its
     * size in height/width (these are equal).
     */
    private static class ThumbnailKey {
        private final String mFilePath;
        private final int mSizePx;

        public ThumbnailKey(String filePath, int sizePx) {
            mFilePath = filePath;
            mSizePx = sizePx;
        }

        public String getFilePath() {
            return mFilePath;
        }

        public int getSizePx() {
            return mSizePx;
        }

        @Override
        public boolean equals(Object other) {
            if (!(other instanceof ThumbnailKey)) return false;
            ThumbnailKey otherKey = (ThumbnailKey) other;
            return otherKey.getFilePath() == mFilePath && otherKey.getSizePx() == mSizePx;
        }
    }

    /**
     * Holds the value for the cached thumbnails. Consists of the {@link Bitmap} of the thumbnail
     * and its size in byte count.
     */
    private static class ThumbnailValue {
        private Bitmap mBitmap;
        private int mByteCount;

        public ThumbnailValue(Bitmap bitmap, int byteCount) {
            mBitmap = bitmap;
            mByteCount = byteCount;
        }

        public Bitmap getBitmap() {
            return mBitmap;
        }

        public int getSize() {
            return mByteCount;
        }
    }

    private native long nativeInit();
    private native void nativeDestroy(long nativeThumbnailProvider);
    private native void nativeRetrieveThumbnail(
            long nativeThumbnailProvider, String filePath, int thumbnailSize);
}
