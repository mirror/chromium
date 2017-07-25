// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;

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
import java.util.LinkedHashMap;
import java.util.Map;

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
    // LRU cache that maps content ID to ThumbnailEntry. The order is based on the sequence of add
    // and get with the most recent entry at the end. The order at initialization (i.e. browser
    // restart) is based on the order of files in the directory.
    @VisibleForTesting
    static final LinkedHashMap<String, ThumbnailEntry> sDiskLruCache =
            new LinkedHashMap<String, ThumbnailEntry>();

    @VisibleForTesting
    final ThumbnailGenerator mThumbnailGenerator;

    private ThumbnailProviderImpl mRequester;

    @VisibleForTesting
    File mDirectory;

    // Number of bytes used in disk for cache (bitmap and ThumbnailEntry).
    @VisibleForTesting
    int mSize;

    /**
     * Contains information related to the thumbnail being cached.
     * Accessed in @{link ThumbnailDiskStorageTest#getMostRecentEntry()}
     */
    @VisibleForTesting
    class ThumbnailEntry {
        public final String contentId;
        public final int size;
        public final String thumbnailFilePath;

        public ThumbnailEntry(String contentId, int thumbnailSize, String thumbnailFilePath) {
            this.contentId = contentId;
            this.size = thumbnailSize;
            this.thumbnailFilePath = thumbnailFilePath;
        }

        public int getByteSize() {
            return contentId.length() + Integer.BYTES + thumbnailFilePath.length();
        }
    }

    private class InitTask extends AsyncTask<Void, Void, Void> {
        protected Void doInBackground(Void... params) {
            initDiskCache();
            return null;
        }
    }

    /**
     * Writes to disk cache if necessary and returns thumbnail to requester.
     */
    @VisibleForTesting
    class CacheAndReturnThumbnailTask extends AsyncTask<Void, Void, Void> {
        private boolean mShouldCache;
        private String mContentId;
        private Bitmap mBitmap;

        public CacheAndReturnThumbnailTask(boolean shouldCache, String contentId, Bitmap bitmap) {
            mShouldCache = shouldCache;
            mContentId = contentId;
            mBitmap = bitmap;
        }

        @Override
        protected Void doInBackground(Void... params) {
            // Cache to disk if the request's content ID is not null
            if (mShouldCache && mBitmap != null) {
                addEntryToDisk(mContentId, mBitmap, mBitmap.getByteCount());
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void v) {
            mRequester.onThumbnailRetrieved(mContentId, mBitmap);
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
            if (mRequest.shouldCacheToDisk()
                    && sDiskLruCache.get(mRequest.getContentId()) != null) {
                return getFromDisk(mRequest.getContentId());
            }
            return null;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            if (bitmap == null) {
                // Asynchronously process the file to make a thumbnail.
                mThumbnailGenerator.retrieveThumbnail(mRequest);
                return;
            }
            onThumbnailRetrieved(true, mRequest.getContentId(), bitmap);
        }
    }

    @VisibleForTesting
    ThumbnailDiskStorage(ThumbnailProviderImpl requester, ThumbnailGenerator thumbnailGenerator) {
        mRequester = requester;
        mThumbnailGenerator = thumbnailGenerator;
        mThumbnailGenerator.setRequester(this);
        new InitTask().execute();
    }

    /**
     * Constructs an instance of ThumbnailDiskStorage with the {@link ThumbnailProviderImpl} to call
     * back to.
     * @param requester     The {@link ThumbnailProviderImpl} that makes requests.
     * @return
     */
    public static ThumbnailDiskStorage create(ThumbnailProviderImpl requester) {
        return new ThumbnailDiskStorage(requester, new ThumbnailGenerator());
    }

    /**
     * Destroys the {@link ThumbnailGenerator}.
     */
    public void destroy() {
        mThumbnailGenerator.destroy();
    }

    /**
     * Retrieves the requested thumbnail.
     * @param request       The request for the thumbnail
     */
    public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request) {
        if (request.getContentId() == "") return;

        new GetThumbnailTask(request).execute();
    }

    /**
     * Called when thumbnail is ready, either retrieved from disk or generated by
     * {@link ThumbnailGenerator}.
     * @param request       The request for the thumbnail.
     * @param bitmap        The thumbnail requested.
     */
    public void onThumbnailRetrieved(boolean shouldCache, String contentId, Bitmap bitmap) {
        ThreadUtils.assertOnUiThread();
        new CacheAndReturnThumbnailTask(shouldCache, contentId, bitmap).execute();
    }

    /**
     * Read cached @{link ThumbnailEntry} from disk.
     */
    @VisibleForTesting
    void initDiskCache() {
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
                int size = ois.readInt();
                String thumbnailFilePath = (String) ois.readObject();

                ThumbnailEntry newEntry = new ThumbnailEntry(contentId, size, thumbnailFilePath);
                sDiskLruCache.put(contentId, newEntry);
            } catch (ClassNotFoundException | IOException e) {
                Log.e(TAG, "Error while reading from disk", e);
            } finally {
                StreamUtil.closeQuietly(ois);
                StreamUtil.closeQuietly(fis);
            }
        }
    }

    /**
     * Adds thumbnail entry to disk as most recent. Entry with an existing content ID will be
     * replaced. Called by background thread.
     * @param contentId
     * @param bitmap
     * @param thumbnailSize
     */
    // TODO(angelashao): Use a DB to store all fields in {@link ThumbnailEntry} (crbug.com/747555)
    @VisibleForTesting
    void addEntryToDisk(String contentId, Bitmap bitmap, int thumbnailSize) {
        if (bitmap == null) return;

        String thumbnailFilePath = createFilePath(contentId);

        if (sDiskLruCache.containsKey(contentId)) {
            boolean isRemoved = remove(contentId) != null;
            assert isRemoved;
        }

        ThumbnailEntry newEntry = new ThumbnailEntry(contentId, thumbnailSize, thumbnailFilePath);
        sDiskLruCache.put(contentId, newEntry);

        FileOutputStream fos = null;
        ObjectOutputStream oos = null;
        try {
            fos = new FileOutputStream(new File(thumbnailFilePath));
            oos = new ObjectOutputStream(fos);
            oos.writeObject(contentId);
            oos.writeInt(thumbnailSize);
            oos.writeObject(thumbnailFilePath);
            bitmap.compress(
                    Bitmap.CompressFormat.PNG, 100, oos); // Write compressed bitmap to stream
            oos.flush();
        } catch (IOException e) {
            Log.e(TAG, "Error while writing to disk", e);
        } finally {
            StreamUtil.closeQuietly(oos);
            StreamUtil.closeQuietly(fos);
        }

        mSize += (thumbnailSize + newEntry.getByteSize());
        trim();
    }

    /**
     * Retrieves bitmap with the contentId from cache. Called by background thread.
     * @param contentId
     * @return Bitmap
     */
    @VisibleForTesting
    Bitmap getFromDisk(String contentId) {
        if (!sDiskLruCache.containsKey(contentId)) return null;

        Bitmap bitmap = null;
        ThumbnailEntry cachedEntry = sDiskLruCache.get(contentId);
        FileInputStream fis = null;
        ObjectInputStream ois = null;
        try {
            fis = new FileInputStream(new File(cachedEntry.thumbnailFilePath));
            ois = new ObjectInputStream(fis);
            ois.readObject();
            ois.readInt();
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
    private void trim() {
        while (mSize > MAX_CACHE_BYTES) {
            Map.Entry<String, ThumbnailEntry> toEvict = sDiskLruCache.entrySet().iterator().next();
            remove(toEvict.getKey());
        }
    }

    /**
     * Remove entry with the given contentId from the disk.
     * @param contentId
     * @return ThumbnailEntry that is removed
     */
    @VisibleForTesting
    ThumbnailEntry remove(String contentId) {
        if (!sDiskLruCache.containsKey(contentId)) return null;

        ThumbnailEntry removed = sDiskLruCache.remove(contentId);
        File file = new File(removed.thumbnailFilePath);

        try {
            if (!file.delete()) {
                Log.e(TAG, "Error while removing from disk");
            }
        } catch (SecurityException se) {
            Log.e(TAG, "Error while removing from disk", se);
        }
        mSize -= (removed.size + removed.getByteSize());
        return removed;
    }

    @VisibleForTesting
    String createFilePath(String filename) {
        return mDirectory.getPath() + File.separator + filename;
    }

    /**
     * The number of entries in the disk cache.
     */
    @VisibleForTesting
    int getCacheCount() {
        return sDiskLruCache.size();
    }

    /**
     * Create a unique directory of the designated app (internal) cache directory.
     * @param context
     * @param uniqueName
     * @return File
     */
    private static File getDiskCacheDir(Context context, String uniqueName) {
        return new File(context.getCacheDir().getPath() + File.separator + uniqueName);
    }
}