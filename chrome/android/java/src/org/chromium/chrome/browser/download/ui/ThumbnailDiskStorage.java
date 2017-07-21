// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.StrictMode;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * This class is a LRU cache of thumbnails on the disk. Thumbnails are shared across all
 * ThumbnailProviderDiskStorages. All AsyncTasks are executed serially.
 *
 * Missing thumbnails are retrieved by the ThumbnailProviderGenerator.
 *
 * The caller should use @{link ThumbnailDiskStorage#create()} to create an instance.
 */
public class ThumbnailDiskStorage
        implements ThumbnailProviderImplDelegate, ThumbnailGeneratorObserver {
    private static final String TAG = "ThumbnailStorage";
    private static final int MAX_CACHE_BYTES = 1024 * 1024; // Max disk cache size is 1MB
    // LRU cache that maps content ID to ThumbnailEntry. Most recent entry is at the end. It is
    // accessed only in the background thread (in AsyncTasks)
    private static final LinkedHashMap<String, ThumbnailEntry> sDiskLruCache =
            new LinkedHashMap<String, ThumbnailEntry>();

    @VisibleForTesting
    final ThumbnailGenerator mThumbnailGenerator;

    private ThumbnailProvider.ThumbnailRequest mRequest;
    private ThumbnailGeneratorObserver mObserver;

    @VisibleForTesting
    File mDirectory;

    // Number of bytes used in disk for cache (bitmap and ThumbnailEntry).
    @VisibleForTesting
    int mSize;

    private class InitTask extends AsyncTask<Void, Void, Void> {
        protected Void doInBackground(Void... params) {
            initDiskCache();
            return null;
        }
    }

    @VisibleForTesting
    class DiskWriteTask extends AsyncTask<Object, Void, Void> {
        @Override
        protected Void doInBackground(Object... params) {
            String contentId = (String) params[0];
            Bitmap bitmap = (Bitmap) params[1];
            addEntryToDisk(contentId, bitmap, bitmap.getByteCount());
            return null;
        }
    }

    private class DiskReadTask extends AsyncTask<Object, Void, Void> {
        private String mFilePath;
        private ThumbnailDiskStorage mStorage;

        @Override
        protected Void doInBackground(Object... params) {
            ThumbnailProvider.ThumbnailRequest request =
                    (ThumbnailProvider.ThumbnailRequest) params[0];
            if (sDiskLruCache.get(mRequest.getContentId()) == null) {
                // Asynchronously process the file to make a thumbnail.
                mThumbnailGenerator.retrieveThumbnail(request, ThumbnailDiskStorage.this);
                return null;
            }
            mFilePath = request.getFilePath();
            String contentId = request.getContentId();
            mStorage = (ThumbnailDiskStorage) params[1];
            onThumbnailRetrieved(mFilePath, mStorage.get(contentId));
            return null;
        }
    }

    @VisibleForTesting
    ThumbnailDiskStorage(ThumbnailGenerator thumbnailGenerator) {
        mThumbnailGenerator = thumbnailGenerator;
        new InitTask().execute();
    }

    public static ThumbnailDiskStorage create() {
        return new ThumbnailDiskStorage(new ThumbnailGenerator());
    }

    @Override
    public void destroy() {
        mThumbnailGenerator.destroy();
    }

    @Override
    public void retrieveThumbnail(
            ThumbnailProvider.ThumbnailRequest request, ThumbnailGeneratorObserver observer) {
        if (request.getContentId() == "") return;

        mObserver = observer;
        mRequest = request;
        new DiskReadTask().execute(mRequest, this);
    }

    @Override
    public void onThumbnailRetrieved(String filePath, Bitmap bitmap) {
        getWriteTask().execute(mRequest.getContentId(), bitmap);
        mRequest = null;
        mObserver.onThumbnailRetrieved(filePath, bitmap);
    }

    @VisibleForTesting
    DiskWriteTask getWriteTask() {
        return new DiskWriteTask();
    }

    /**
     * Read cached ThumbnailEntry's from disk.
     */
    @VisibleForTesting
    void initDiskCache() {
        mDirectory = getDiskCacheDir(ContextUtils.getApplicationContext(), "thumbnails");
        if (!mDirectory.exists()) {
            mDirectory.mkdir();
        }

        for (File file : mDirectory.listFiles()) {
            FileInputStream fis;
            ObjectInputStream ois;
            try {
                fis = new FileInputStream(file);
                ois = new ObjectInputStream(fis);
                String contentId = (String) ois.readObject();
                Bitmap bitmap = BitmapFactory.decodeStream(ois);
                int size = ois.readInt();
                String thumbnailFilePath = (String) ois.readObject();

                ThumbnailEntry newEntry =
                        new ThumbnailEntry(contentId, bitmap.getByteCount(), thumbnailFilePath);
                sDiskLruCache.put(contentId, newEntry);

                fis.close();
                ois.close();
            } catch (FileNotFoundException fnfe) {
                Log.e(TAG, "Error while reading from disk", fnfe);
            } catch (IOException e) {
                Log.e(TAG, "Error while reading from disk", e);
            } catch (ClassNotFoundException cnfe) {
                Log.e(TAG, "Error while reading from disk", cnfe);
            }
        }
    }

    /**
     * Adds thumbnail entry to disk as most recent. Entry with an existing content ID will be
     * replaced.
     */
    @VisibleForTesting
    void addEntryToDisk(String contentId, Bitmap bitmap, int thumbnailSize) {
        if (bitmap == null) return;

        String thumbnailFilePath = createFilePath(contentId);

        if (sDiskLruCache.containsKey(contentId)) {
            assert remove(contentId) != null;
        }

        ThumbnailEntry newEntry = new ThumbnailEntry(contentId, thumbnailSize, thumbnailFilePath);
        sDiskLruCache.put(contentId, newEntry);

        FileOutputStream fos;
        ObjectOutputStream oos;
        try {
            fos = new FileOutputStream(new File(thumbnailFilePath));
            oos = new ObjectOutputStream(fos);
            oos.writeObject(contentId);
            bitmap.compress(
                    Bitmap.CompressFormat.PNG, 100, oos); // Write compressed bitmap to stream
            oos.writeInt(thumbnailSize);
            oos.writeObject(thumbnailFilePath);
            oos.flush();
            oos.close();
        } catch (FileNotFoundException fnfe) {
            Log.e(TAG, "Error while writing to disk", fnfe);
        } catch (IOException e) {
            Log.e(TAG, "Error while writing to disk", e);
        }

        mSize += (thumbnailSize + contentId.length() + Integer.BYTES + thumbnailFilePath.length());
        if (mSize > MAX_CACHE_BYTES) {
            trim();
        }
    }

    /**
     * Retrieves bitmap with the contentId from cache.
     */
    @VisibleForTesting
    Bitmap get(String contentId) {
        if (!sDiskLruCache.containsKey(contentId)) return null;

        Bitmap bitmap = null;
        ThumbnailEntry cachedEntry = sDiskLruCache.get(contentId);
        try {
            FileInputStream fis = new FileInputStream(new File(cachedEntry.thumbnailFilePath));
            ObjectInputStream ois = new ObjectInputStream(fis);
            ois.readObject();
            bitmap = BitmapFactory.decodeStream(ois);
            ois.close();
            fis.close();
        } catch (FileNotFoundException fnfe) {
            Log.e(TAG, "Error while reading from disk", fnfe);
        } catch (IOException e) {
            Log.e(TAG, "Error while reading from disk", e);
        } catch (ClassNotFoundException cnfe) {
            Log.e(TAG, "Error while reading from disk", cnfe);
        }

        return bitmap;
    }

    /**
     * Trim the cache to stay under the MAX_CACHE_BYTES limit by removing the oldest entries.
     */
    @VisibleForTesting
    void trim() {
        while (mSize > MAX_CACHE_BYTES) {
            Map.Entry<String, ThumbnailEntry> toEvict = sDiskLruCache.entrySet().iterator().next();
            remove(toEvict.getKey());
        }
    }

    /**
     * Remove entry with the given contentId from the disk.
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
        mSize -= (removed.size + removed.contentId.length() + Integer.BYTES
                + removed.thumbnailFilePath.length());
        return removed;
    }

    /**
     * Create a unique directory of the designated app cache directory. Tries to use external
     * but if not mounted, falls back on internal storage.
     */
    private static File getDiskCacheDir(Context context, String uniqueName) {
        // Check if media is mounted or storage is built-in, if so, try and use external cache dir
        // otherwise use internal cache dir
        String externalStorageState;
        File externalCacheDir;
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            externalStorageState = Environment.getExternalStorageState();
            externalCacheDir = context.getExternalCacheDir();
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
        final String cachePath = Environment.MEDIA_MOUNTED.equals(externalStorageState)
                        || !Environment.isExternalStorageRemovable()
                ? externalCacheDir.getPath()
                : context.getCacheDir().getPath();

        return new File(cachePath + File.separator + uniqueName);
    }

    @VisibleForTesting
    String createFilePath(String filename) {
        return mDirectory.getPath() + File.separator + filename;
    }

    @VisibleForTesting
    ThumbnailEntry getOldestEntry() {
        if (getCacheCount() <= 0) return null;

        return sDiskLruCache.entrySet().iterator().next().getValue();
    }

    @VisibleForTesting
    ThumbnailEntry getMostRecentEntry() {
        if (getCacheCount() <= 0) return null;

        ArrayList<Map.Entry<String, ThumbnailEntry>> entryList =
                new ArrayList<Map.Entry<String, ThumbnailEntry>>(sDiskLruCache.entrySet());
        return entryList.get(entryList.size() - 1).getValue();
    }

    /**
     * The number of entries in the disk cache.
     */
    @VisibleForTesting
    int getCacheCount() {
        return sDiskLruCache.size();
    }
}