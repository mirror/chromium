// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.support.v4.util.AtomicFile;
import android.support.v4.util.Pair;
import android.text.TextUtils;

import com.google.protobuf.nano.MessageNano;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.StreamUtil;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.download.ui.ThumbnailCacheEntry.ContentId;
import org.chromium.chrome.browser.download.ui.ThumbnailCacheEntry.ThumbnailEntry;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;

/**
 * This class is a LRU cache of thumbnails on the disk and calls back to
 * {@link ThumbnailProviderImpl}. Thumbnails are shared across all
 * {@link ThumbnailProviderDiskStorage}s. There can be multiple
 * @{link ThumbnailProvider.ThumbnailRequest} being processed at a time, but all AsyncTasks are
 * executed serially. Missing thumbnails are retrieved by the ThumbnailProviderGenerator.
 *
 * The caller should use @{link ThumbnailDiskStorage#create()} to create an instance.
 *
 * This class removes thumbnails from disk only if the file was removed in Download Home. It relies
 * on restart (when initDiskCache is called) and trim to sync to disk if file was removed
 * elsewhere (e.g. manually from disk).
 */
public class ThumbnailDiskStorage implements ThumbnailGeneratorCallback {
    private static final String TAG = "ThumbnailStorage";
    private static final int MAX_CACHE_BYTES = 1024 * 1024; // Max disk cache size is 1MB
    // LRU cache of a pair of thumbnail's contentID and size. The order is based on the sequence of
    // add and get with the most recent at the end. The order at initialization (i.e. browser
    // restart) is based on the order of files in the directory. It is accessed only on the
    // background thread.
    // It is static because cached thumbnails are shared across all instances of the class.
    @VisibleForTesting
    static final LinkedHashSet<Pair<String, Integer>> sDiskLruCache =
            new LinkedHashSet<Pair<String, Integer>>();

    // Maps content ID to a set of the sizes of the thumbnail with that ID
    @VisibleForTesting
    static final HashMap<String, HashSet<Integer>> sIconSizesMap =
            new HashMap<String, HashSet<Integer>>();

    @VisibleForTesting
    final ThumbnailGenerator mThumbnailGenerator;

    // This should be initialized once.
    @VisibleForTesting
    private File mDirectory;

    private ThumbnailRetrievalCallback mCallback;

    // Number of bytes used in disk for cache.
    @VisibleForTesting
    long mSize;

    private class InitTask extends AsyncTask<Void, Void, Void> {
        protected Void doInBackground(Void... params) {
            initDiskCache();
            return null;
        }
    }

    /**
     * Writes to disk cache.
     */
    @VisibleForTesting
    class CacheThumbnailTask extends AsyncTask<Void, Void, Void> {
        private boolean mShouldCache;
        private String mContentId;
        private Bitmap mBitmap;
        private int mIconWidthPx;

        public CacheThumbnailTask(String contentId, Bitmap bitmap, int iconWidthPx) {
            mContentId = contentId;
            mBitmap = bitmap;
            mIconWidthPx = iconWidthPx;
        }

        @Override
        protected Void doInBackground(Void... params) {
            addToDisk(mContentId, mBitmap, mIconWidthPx);
            return null;
        }
    }

    /**
     * Reads from disk cache. If missing, fetch from {@link ThumbnailGenerator}.
     */
    private class GetThumbnailTask extends AsyncTask<Void, Void, Bitmap> {
        private ThumbnailProvider.ThumbnailRequest mRequest;

        public GetThumbnailTask(ThumbnailProvider.ThumbnailRequest request) {
            mRequest = request;
        }

        @Override
        protected Bitmap doInBackground(Void... params) {
            if (sDiskLruCache.contains(
                        Pair.create(mRequest.getContentId(), mRequest.getIconSize()))) {
                return getFromDisk(mRequest.getContentId(), mRequest.getIconSize());
            }
            return null;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            if (bitmap != null) {
                onThumbnailRetrieved(true, mRequest.getContentId(), bitmap, mRequest.getIconSize());
                return;
            }
            // Asynchronously process the file to make a thumbnail.
            mThumbnailGenerator.retrieveThumbnail(mRequest, ThumbnailDiskStorage.this);
        }
    }

    @VisibleForTesting
    ThumbnailDiskStorage(
            ThumbnailRetrievalCallback callback, ThumbnailGenerator thumbnailGenerator) {
        ThreadUtils.assertOnUiThread();
        mCallback = callback;
        mThumbnailGenerator = thumbnailGenerator;
        new InitTask().executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
    }

    /**
     * Constructs an instance of ThumbnailDiskStorage with the {@link ThumbnailRetrievalCallback} to
     * call back to.
     * @param requester The {@link ThumbnailRetrievalCallback} that makes requests.
     * @return
     */
    public static ThumbnailDiskStorage create(ThumbnailRetrievalCallback callback) {
        return new ThumbnailDiskStorage(callback, new ThumbnailGenerator());
    }

    /**
     * Destroys the {@link ThumbnailGenerator}.
     */
    public void destroy() {
        mThumbnailGenerator.destroy();
    }

    /**
     * Retrieves the requested thumbnail.
     * @param request The request for the thumbnail
     */
    public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request) {
        ThreadUtils.assertOnUiThread();
        if (TextUtils.isEmpty(request.getContentId())) return;

        if (request.shouldCacheToDisk()) {
            new GetThumbnailTask(request).executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
            return;
        }
        mThumbnailGenerator.retrieveThumbnail(request, this);
    }

    /**
     * Called when thumbnail is ready, either retrieved from disk or generated by
     * {@link ThumbnailGenerator}.
     * @param shouldCache Whether the retrieved bitmap should be cached to disk.
     * @param request The request for the thumbnail.
     * @param bitmap The thumbnail requested.
     * @param iconWidthPx The width (pixels) of the thumbnail requested.
     */
    public void onThumbnailRetrieved(
            boolean shouldCache, String contentId, Bitmap bitmap, int iconWidthPx) {
        ThreadUtils.assertOnUiThread();
        if (shouldCache && bitmap != null && !TextUtils.isEmpty(contentId)) {
            new CacheThumbnailTask(contentId, bitmap, iconWidthPx)
                    .executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
        }
        mCallback.onThumbnailRetrieved(contentId, bitmap);
    }

    /**
     * Read previously cached thumbnail-related info from disk. Initialize only once. Invoked on
     * background thread.
     */
    @VisibleForTesting
    void initDiskCache() {
        if (isInitialized()) return;

        ThreadUtils.assertOnBackgroundThread();
        mDirectory = getDiskCacheDir(ContextUtils.getApplicationContext(), "thumbnails");
        if (!mDirectory.exists()) {
            mDirectory.mkdir();
        }

        for (File file : mDirectory.listFiles()) {
            AtomicFile atomicFile = new AtomicFile(file);
            try {
                ThumbnailEntry entry =
                        MessageNano.mergeFrom(new ThumbnailEntry(), atomicFile.readFully());
                if (entry.contentId == null) continue;

                String contentId = entry.contentId.id;
                int iconSize = entry.widthPx;
                sDiskLruCache.add(Pair.create(contentId, iconSize));
                if (sIconSizesMap.containsKey(contentId)) {
                    sIconSizesMap.get(contentId).add(iconSize);
                } else {
                    HashSet<Integer> set = new HashSet<Integer>();
                    set.add(iconSize);
                    sIconSizesMap.put(contentId, set);
                }
                mSize += file.length();
            } catch (IOException e) {
                Log.e(TAG, "Error while reading from disk", e);
            }
        }
    }

    /**
     * Adds thumbnail to disk as most recent. Thumbnail with an existing content ID in cache will be
     * replaced by the newly added. Invoked on background thread.
     * @param contentId Content ID for the thumbnail to cache to disk.
     * @param bitmap The thumbnail to cache.
     * @param iconWidthPx The width (pixel) of the thumbnail.
     */
    // TODO(angelashao): Use a DB to store thumbnail-related data. (crbug.com/747555)
    @VisibleForTesting
    void addToDisk(String contentId, Bitmap bitmap, int iconWidthPx) {
        ThreadUtils.assertOnBackgroundThread();
        assert isInitialized();

        if (sDiskLruCache.contains(Pair.create(contentId, iconWidthPx))) {
            removeFromDisk(contentId);
        }

        FileOutputStream fos = null;
        AtomicFile atomicFile = null;
        try {
            File newFile = new File(getThumbnailFilePath(contentId, iconWidthPx));
            atomicFile = new AtomicFile(newFile);
            fos = atomicFile.startWrite();

            ContentId newEntryContentId = new ContentId();
            newEntryContentId.id = contentId;

            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, baos);
            byte[] byteArray = baos.toByteArray();

            ThumbnailEntry newEntry = new ThumbnailEntry();
            newEntry.contentId = newEntryContentId;
            newEntry.widthPx = iconWidthPx;
            newEntry.compressedPng = byteArray;
            byte[] entryByteArray = MessageNano.toByteArray(newEntry);
            fos.write(entryByteArray);
            atomicFile.finishWrite(fos);

            sDiskLruCache.add(Pair.create(contentId, iconWidthPx));
            if (sIconSizesMap.containsKey(contentId)) {
                sIconSizesMap.get(contentId).add(iconWidthPx);
            } else {
                HashSet<Integer> set = new HashSet<Integer>();
                set.add(iconWidthPx);
                sIconSizesMap.put(contentId, set);
            }
            mSize += newFile.length();
            trim();
        } catch (IOException e) {
            Log.e(TAG, "Error while writing to disk", e);
            atomicFile.failWrite(fos);
        }
    }

    private boolean isInitialized() {
        return mDirectory != null;
    }

    /**
     * Retrieves bitmap with the contentId from cache. Invoked on background thread.
     * @param contentId The content ID of the requested thumbnail.
     * @param iconWidthPx The width (pixel) of the requested thumbnail.
     * @return Bitmap
     */
    @VisibleForTesting
    Bitmap getFromDisk(String contentId, int iconWidthPx) {
        ThreadUtils.assertOnBackgroundThread();
        if (!sDiskLruCache.contains(Pair.create(contentId, iconWidthPx))) return null;

        Bitmap bitmap = null;
        FileInputStream fis = null;
        try {
            String thumbnailFilePath = getThumbnailFilePath(contentId, iconWidthPx);
            File file = new File(thumbnailFilePath);
            // If file doesn't exist, we cannot update mSize to account for the removal but this is
            // fine in the long-run when trim happens.
            if (!file.exists()) {
                return null;
            }

            AtomicFile atomicFile = new AtomicFile(file);
            fis = atomicFile.openRead();
            ThumbnailEntry entry =
                    MessageNano.mergeFrom(new ThumbnailEntry(), atomicFile.readFully());
            bitmap = BitmapFactory.decodeByteArray(
                    entry.compressedPng, 0, entry.compressedPng.length);
        } catch (IOException e) {
            Log.e(TAG, "Error while reading from disk", e);
        } finally {
            StreamUtil.closeQuietly(fis);
        }

        return bitmap;
    }

    /**
     * Trim the cache to stay under the MAX_CACHE_BYTES limit by removing the oldest entries.
     */
    @VisibleForTesting
    void trim() {
        ThreadUtils.assertOnBackgroundThread();
        while (mSize > MAX_CACHE_BYTES) {
            removeFromDiskHelper(sDiskLruCache.iterator().next());
        }
    }

    /**
     * May be out of sync with disk. If the thumbnail is not on disk, return false.
     * @param contentIdSizePair
     * @return Whether the thumbnail has been successfully deleted from disk.
     */
    @VisibleForTesting
    boolean removeFromDiskHelper(Pair<String, Integer> contentIdSizePair) {
        if (!sDiskLruCache.contains(contentIdSizePair)) return false;

        String contentId = contentIdSizePair.first;
        int iconSize = contentIdSizePair.second;
        File file = new File(getThumbnailFilePath(contentId, iconSize));
        if (!file.exists()) return false;

        AtomicFile atomicFile = new AtomicFile(file);
        boolean isRemoved = false;
        try {
            long fileSize = file.length();
            atomicFile.delete();
            isRemoved = sDiskLruCache.remove(contentIdSizePair);
            sIconSizesMap.get(contentId).remove(iconSize);
            if (sIconSizesMap.get(contentId).size() == 0) {
                sIconSizesMap.remove(contentId);
            }
            mSize -= fileSize;
        } catch (SecurityException se) {
            Log.e(TAG, "Error while removing from disk", se);
        }
        return isRemoved;
    }

    /**
     * Remove thumbnails with the given contentId from disk. Returns false if {@code contentId}
     * doesn't exist in cache or files cannot be successfully deleted from disk.
     * @param contentId
     * @return Whether removal was successful.
     */
    public boolean removeFromDisk(String contentId) {
        if (!sIconSizesMap.containsKey(contentId)) return false;

        boolean areRemoved = true;
        ArrayList<Integer> iconSizes = new ArrayList<Integer>(sIconSizesMap.get(contentId));
        for (int iconSize : iconSizes) {
            areRemoved = areRemoved && removeFromDiskHelper(Pair.create(contentId, iconSize));
        }
        return areRemoved;
    }

    /**
     * Gets the file path for a thumbnail with the given content ID and size.
     * @param contentId Content ID for the thumbnail.
     * @param iconSize The width of the thumbnail.
     * @return File path.
     */
    private String getThumbnailFilePath(String contentId, int iconSize) {
        return mDirectory.getPath() + File.separator + contentId + iconSize + ".entry";
    }

    /**
     * Get directory for thumbnail entries in the designated app (internal) cache directory.
     * The directory's name must be unique.
     * @param context The application's context.
     * @param uniqueName The name of the thumbnail directory. Must be unique.
     * @return The path to the thumbnail cache directory.
     */
    private static File getDiskCacheDir(Context context, String thumbnailDirName) {
        return new File(context.getCacheDir().getPath() + File.separator + thumbnailDirName);
    }
}