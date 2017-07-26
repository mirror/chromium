// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.StreamUtil;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.LinkedHashSet;

/**
 * This class is a LRU cache of thumbnails on the disk and calls back to
 * {@link ThumbnailProviderImpl}. Thumbnails are shared across all ThumbnailProviderDiskStorages.
 * There can be multiple @{link ThumbnailProvider.ThumbnailRequest} being processed at a time, but
 * all AsyncTasks are executed serially.
 *
 * Missing thumbnails are retrieved by the ThumbnailProviderGenerator.
 *
 * The caller should use @{link ThumbnailDiskStorage#create()} to create an instance.
 */
public class ThumbnailDiskStorage {
    private static final String TAG = "ThumbnailStorage";
    private static final int MAX_CACHE_BYTES = 1024 * 1024; // Max disk cache size is 1MB
    // LRU cache of thumbnail's contentID. The order is based on the sequence of add and get with
    // the most recent at the end. The order at initialization (i.e. browser restart) is based on
    // the order of files in the directory. It is accessed only on the background thread.
    @VisibleForTesting
    static final LinkedHashSet<String> sDiskLruCache = new LinkedHashSet<String>();

    @VisibleForTesting
    final ThumbnailGenerator mThumbnailGenerator;

    private ThumbnailProviderCallback mCallback;

    @VisibleForTesting
    File mDirectory;

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

        public CacheThumbnailTask(String contentId, Bitmap bitmap) {
            mContentId = contentId;
            mBitmap = bitmap;
        }

        @Override
        protected Void doInBackground(Void... params) {
            addToDisk(mContentId, mBitmap);
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
            if (sDiskLruCache.contains(mRequest.getContentId())) {
                return getFromDisk(mRequest.getContentId());
            }
            return null;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            if (bitmap != null) {
                onThumbnailRetrieved(true, mRequest.getContentId(), bitmap);
                return;
            }
            // Asynchronously process the file to make a thumbnail.
            mThumbnailGenerator.retrieveThumbnail(mRequest);
        }
    }

    @VisibleForTesting
    ThumbnailDiskStorage(
            ThumbnailProviderCallback callback, ThumbnailGenerator thumbnailGenerator) {
        mCallback = callback;
        mThumbnailGenerator = thumbnailGenerator;
        mThumbnailGenerator.setRequester(this);
        // Initialize once only
        if (mDirectory == null) {
            new InitTask().execute();
        }
    }

    /**
     * Constructs an instance of ThumbnailDiskStorage with the {@link ThumbnailProviderCallback} to
     * call back to.
     * @param requester The {@link ThumbnailProviderCallback} that makes requests.
     * @return
     */
    public static ThumbnailDiskStorage create(ThumbnailProviderCallback callback) {
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
        if (TextUtils.isEmpty(request.getContentId())) return;

        if (request.shouldCacheToDisk()) {
            new GetThumbnailTask(request).execute();
            return;
        }
        mThumbnailGenerator.retrieveThumbnail(request);
    }

    /**
     * Called when thumbnail is ready, either retrieved from disk or generated by
     * {@link ThumbnailGenerator}.
     * @param shouldCache Whether the retrieved bitmap should be cached to disk.
     * @param request The request for the thumbnail.
     * @param bitmap The thumbnail requested.
     */
    public void onThumbnailRetrieved(boolean shouldCache, String contentId, Bitmap bitmap) {
        ThreadUtils.assertOnUiThread();
        if (shouldCache && bitmap != null && !TextUtils.isEmpty(contentId)) {
            new CacheThumbnailTask(contentId, bitmap).execute();
        }
        mCallback.onThumbnailRetrieved(contentId, bitmap);
    }

    /**
     * Read from disk. Called by background thread.
     */
    @VisibleForTesting
    void initDiskCache() {
        ThreadUtils.assertOnBackgroundThread();
        mDirectory = getDiskCacheDir(ContextUtils.getApplicationContext(), "thumbnails");
        if (!mDirectory.exists()) {
            mDirectory.mkdir();
        }

        for (File file : mDirectory.listFiles()) {
            FileInputStream fis = null;
            ObjectInputStream ois = null;
            try {
                fis = new FileInputStream(file);
                ois = new ObjectInputStream(fis);
                String contentId = (String) ois.readObject();
                sDiskLruCache.add(contentId);
            } catch (ClassNotFoundException | IOException e) {
                Log.e(TAG, "Error while reading from disk", e);
            } finally {
                StreamUtil.closeQuietly(ois);
                StreamUtil.closeQuietly(fis);
            }
        }
    }

    /**
     * Adds thumbnail to disk as most recent. Thumbnail with an existing content ID in cache will be
     * replaced by the newly added. Called by background thread.
     * @param contentId
     * @param bitmap
     */
    // TODO(angelashao): Use a DB to store thumbnail-related data. (crbug.com/747555)
    @VisibleForTesting
    void addToDisk(String contentId, Bitmap bitmap) {
        ThreadUtils.assertOnBackgroundThread();
        if (sDiskLruCache.contains(contentId)) {
            boolean isRemoved = removeFromDisk(contentId);
            assert isRemoved;
        }

        FileOutputStream fos = null;
        ObjectOutputStream oos = null;
        try {
            File newFile = new File(getThumbnailFilePath(contentId));
            fos = new FileOutputStream(newFile);
            oos = new ObjectOutputStream(fos);
            oos.writeObject(contentId);
            bitmap.compress(
                    Bitmap.CompressFormat.PNG, 100, oos); // Write compressed bitmap to stream
            oos.flush();

            sDiskLruCache.add(contentId);
            mSize += newFile.length();
            trim();
        } catch (IOException e) {
            Log.e(TAG, "Error while writing to disk", e);
        } finally {
            StreamUtil.closeQuietly(oos);
            StreamUtil.closeQuietly(fos);
        }
    }

    /**
     * Retrieves bitmap with the contentId from cache. Called by background thread.
     * @param contentId
     * @return Bitmap
     */
    @VisibleForTesting
    Bitmap getFromDisk(String contentId) {
        ThreadUtils.assertOnBackgroundThread();
        if (!sDiskLruCache.contains(contentId)) return null;

        Bitmap bitmap = null;
        FileInputStream fis = null;
        ObjectInputStream ois = null;
        try {
            String thumbnailFilePath = getThumbnailFilePath(contentId);
            File file = new File(thumbnailFilePath);
            boolean exists = file.exists();
            assert exists;
            fis = new FileInputStream(file);
            ois = new ObjectInputStream(fis);
            ois.readObject();
            bitmap = BitmapFactory.decodeStream(ois);
        } catch (ClassNotFoundException | IOException e) {
            Log.e(TAG, "Error while reading from disk", e);
        } finally {
            StreamUtil.closeQuietly(ois);
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
            removeFromDisk(sDiskLruCache.iterator().next());
        }
    }

    /**
     * Remove thumbnail with the given contentId from disk. Returns false if contentId doesn't exist
     * in cache or file cannot be successfully deleted from disk.
     * @param contentId
     * @return Whether removal was successful.
     */
    public boolean removeFromDisk(String contentId) {
        if (!sDiskLruCache.contains(contentId)) return false;

        File file = new File(getThumbnailFilePath(contentId));
        boolean fileExists = file.exists();
        assert fileExists;

        boolean isRemoved = false;
        try {
            long fileSize = file.length();
            boolean deleted = file.delete();
            assert deleted;
            isRemoved = sDiskLruCache.remove(contentId);
            mSize -= fileSize;
        } catch (SecurityException se) {
            Log.e(TAG, "Error while removing from disk", se);
        }
        return isRemoved;
    }

    /**
     * Gets the file path for a thumbnail with the given content ID.
     * @param contentId
     * @return
     */
    @VisibleForTesting
    String getThumbnailFilePath(String contentId) {
        return mDirectory.getPath() + File.separator + contentId + ".png";
    }

    /**
     * Create a unique directory of the designated app (internal) cache directory.
     * @param context The application's context.
     * @param uniqueName The name of the thumbnail directory. Must be unique.
     * @return File
     */
    private static File getDiskCacheDir(Context context, String uniqueName) {
        return new File(context.getCacheDir().getPath() + File.separator + uniqueName);
    }
}