// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.support.v4.util.LruCache;
import android.text.TextUtils;
import android.util.Pair;

import org.chromium.base.ThreadUtils;

import java.lang.ref.WeakReference;
import java.util.ArrayDeque;
import java.util.Deque;

/**
 * Concrete implementation of {@link ThumbnailProvider}.
 *
 * Thumbnails are cached in memory and shared across all ThumbnailProviderImpls. Missing thumbnails
 * are retrieved by either ThumbnailProviderDiskStorage (if the ThumbnailRequest is a
 * DownloadItemView and hence disk cache is needed) or ThumbnailProviderGenerator (if disk cache is
 * not needed). The memory cache is LRU, limited in size, and automatically garbage collected under
 * memory pressure.
 *
 * A queue of requests is maintained in FIFO order.
 *
 * TODO(dfalcantara): Figure out how to send requests simultaneously to the utility process without
 *                    duplicating work to decode the same image for two different requests.
 */
public class ThumbnailProviderImpl
        implements ThumbnailProvider, ThumbnailProviderGeneratorObserver {
    /** 5 MB of thumbnails should be enough for everyone. */
    private static final int MAX_CACHE_BYTES = 5 * 1024 * 1024;

    /**
     *  Weakly referenced cache containing thumbnails that can be deleted under memory pressure.
     *  Key in the cache is a pair of the filepath and the height/width of the thumbnail. Value is
     *  a pair of the thumbnail and its byte size.
     * */
    private static WeakReference<LruCache<String, Pair<Bitmap, Integer>>> sBitmapCache =
            new WeakReference<>(null);

    /** Enqueues requests. */
    private final Handler mHandler;

    /** Queue of files to retrieve thumbnails for. */
    private final Deque<ThumbnailRequest> mRequestQueue;

    /** Request that is currently having its thumbnail retrieved. */
    private ThumbnailRequest mCurrentRequest;

    private ThumbnailProviderImplDelegate mDelegate;

    public ThumbnailProviderImpl() {
        mHandler = new Handler(Looper.getMainLooper());
        mRequestQueue = new ArrayDeque<>();
        mDelegate = ThumbnailProviderDiskStorage.create();
    }

    /** Destroys the native ThumbnailProviderGenerator. */
    @Override
    public void destroy() {
        ThreadUtils.assertOnUiThread();
        mDelegate.destroy();
    }

    @Override
    public void getThumbnail(ThumbnailRequest request) {
        String filePath = request.getFilePath();
        if (TextUtils.isEmpty(filePath)) return;

        Bitmap cachedBitmap =
                getBitmapFromCache(filePath, request.getContentId(), request.getIconSize());
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

    private Bitmap getBitmapFromCache(String filepath, String contentId, int bitmapSizePx) {
        Pair<Bitmap, Integer> cachedPair = getBitmapCache().get(contentId);
        if (cachedPair == null) return null;
        Bitmap cachedBitmap = cachedPair.first;

        if (cachedBitmap == null) return null;
        assert !cachedBitmap.isRecycled();
        return cachedBitmap;
    }

    private void processNextRequest() {
        if (mCurrentRequest != null || mRequestQueue.isEmpty()) return;

        mCurrentRequest = mRequestQueue.poll();
        String currentFilePath = mCurrentRequest.getFilePath();

        Bitmap cachedBitmap = getBitmapFromCache(
                currentFilePath, mCurrentRequest.getContentId(), mCurrentRequest.getIconSize());
        if (cachedBitmap == null) {
            // Asynchronously process the file to make a thumbnail.
            // nativeRetrieveThumbnail(
            //         mNativeThumbnailProvider, currentFilePath, mCurrentRequest.getIconSize());
            if (shouldCacheToDisk(mCurrentRequest)) {
                if (!(mDelegate instanceof ThumbnailProviderDiskStorage)) {
                    mDelegate = ThumbnailProviderDiskStorage.create();
                }
            } else if (!(mDelegate instanceof ThumbnailProviderGenerator)) {
                mDelegate = new ThumbnailProviderGenerator();
            }
            mDelegate.retrieveThumbnail(mCurrentRequest, this);
        } else {
            // Send back the already-processed file.
            onThumbnailRetrieved(currentFilePath, cachedBitmap);
        }
    }

    private boolean shouldCacheToDisk(ThumbnailProvider.ThumbnailRequest request) {
        return (request instanceof DownloadItemView);
    }

    @Override
    // TODO(angelashao): make method package-private
    public void onThumbnailRetrieved(String filePath, Bitmap bitmap) {
        if (bitmap != null) {
            assert mCurrentRequest.getIconSize() == bitmap.getHeight();
            getBitmapCache().put(
                    mCurrentRequest.getContentId(), Pair.create(bitmap, bitmap.getByteCount()));
            mCurrentRequest.onThumbnailRetrieved(filePath, bitmap);
        }

        mCurrentRequest = null;
        processQueue();
    }

    private static LruCache<String, Pair<Bitmap, Integer>> getBitmapCache() {
        ThreadUtils.assertOnUiThread();

        LruCache<String, Pair<Bitmap, Integer>> cache =
                sBitmapCache == null ? null : sBitmapCache.get();
        if (cache != null) return cache;

        // Create a new weakly-referenced cache.
        cache = new LruCache<String, Pair<Bitmap, Integer>>(MAX_CACHE_BYTES) {
            @Override
            protected int sizeOf(String contentId, Pair<Bitmap, Integer> thumbnail) {
                return thumbnail == null ? 0 : thumbnail.second;
            }
        };
        sBitmapCache = new WeakReference<>(cache);
        return cache;
    }
}