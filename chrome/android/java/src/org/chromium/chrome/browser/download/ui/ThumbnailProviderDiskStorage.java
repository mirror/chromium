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
 * ThumbnailProviderDiskStorages.
 *
 * Missing thumbnails are retrieved by the ThumbnailProviderGenerator.
 *
 * The caller should use @{link ThumbnailProviderDiskStorage#create()} to create an instance.
 */
public class ThumbnailProviderDiskStorage
        implements ThumbnailProviderImplDelegate, ThumbnailProviderGeneratorObserver {
    private static final String TAG = "ThumbnailProvDiskSto";
    private static final int MAX_CACHE_BYTES = 1024 * 1024; // Max disk cache size is 1MB
    // LRU cache that maps content ID to ThumbnailEntry. Most recent entry is at the end.
    private static final LinkedHashMap<String, ThumbnailEntry> sDiskLruCache =
            new LinkedHashMap<String, ThumbnailEntry>();

    @VisibleForTesting
    final ThumbnailProviderGenerator mThumbnailGenerator;

    final File mDirectory;

    private ThumbnailProvider.ThumbnailRequest mRequest;
    private ThumbnailProviderGeneratorObserver mObserver;

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
            String filePath = (String) params[1];
            Bitmap bitmap = (Bitmap) params[2];
            add(contentId, bitmap, bitmap.getByteCount(), filePath);
            return null;
        }
    }

    private class DiskReadTask extends AsyncTask<Object, Void, Bitmap> {
        private String mFilePath;
        private ThumbnailProviderDiskStorage mStorage;

        @Override
        protected Bitmap doInBackground(Object... params) {
            mFilePath = (String) params[0];
            String contentId = (String) params[1];
            mStorage = (ThumbnailProviderDiskStorage) params[2];
            return mStorage.get(contentId);
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            // Send back the already-processed thumbnail.
            onThumbnailRetrieved(mFilePath, bitmap);
        }
    }

    @VisibleForTesting
    ThumbnailProviderDiskStorage(ThumbnailProviderGenerator thumbnailGenerator) {
        mThumbnailGenerator = thumbnailGenerator;
        mDirectory = getDiskCacheDir(ContextUtils.getApplicationContext(), "thumbnails");
        new InitTask().execute();
    }

    public static ThumbnailProviderDiskStorage create() {
        return new ThumbnailProviderDiskStorage(new ThumbnailProviderGenerator());
    }

    @Override
    public void destroy() {
        mThumbnailGenerator.destroy();
    }

    @Override
    public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request,
            ThumbnailProviderGeneratorObserver observer) {
        mObserver = observer;
        mRequest = request;
        synchronized (sDiskLruCache) {
            if (sDiskLruCache.get(mRequest.getContentId()) != null) {
                new DiskReadTask().execute(mRequest.getFilePath(), mRequest.getContentId(), this);
                return;
            }
        }
        // Asynchronously process the file to make a thumbnail.
        mThumbnailGenerator.retrieveThumbnail(request, this);
    }

    @Override
    public void onThumbnailRetrieved(String filePath, Bitmap bitmap) {
        getWriteTask().execute(mRequest.getContentId(), filePath, bitmap);
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
    private synchronized void initDiskCache() {
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
            } catch (ClassNotFoundException e) {
                Log.e(TAG, "Error while reading from disk", e);
            }
        }
    }

    /**
     * Adds entry to disk as most recent. Entry with an existing content ID will be replaced.
     */
    @VisibleForTesting
    synchronized void add(String contentId, Bitmap bitmap, int thumbnailSize, String filePath) {
        if (contentId.equals("") || bitmap == null) return;

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
    synchronized Bitmap get(String contentId) {
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
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "Error while reading from disk", e);
        }

        return bitmap;
    }

    /**
     * Trim the cache to stay under the MAX_CACHE_BYTES limit by removing the oldest entries.
     */
    @VisibleForTesting
    synchronized void trim() {
        while (mSize > MAX_CACHE_BYTES) {
            Map.Entry<String, ThumbnailEntry> toEvict = sDiskLruCache.entrySet().iterator().next();
            remove(toEvict.getKey());
        }
    }

    /**
     * Remove entry with the given contentId from the disk.
     */
    @VisibleForTesting
    synchronized ThumbnailEntry remove(String contentId) {
        if (!sDiskLruCache.containsKey(contentId)) return null;

        int oldCacheCount = getCacheCount();
        ThumbnailEntry removed = sDiskLruCache.remove(contentId);
        assert getCacheCount() == oldCacheCount - 1;
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
    synchronized ThumbnailEntry getOldestEntry() {
        return sDiskLruCache.entrySet().iterator().next().getValue();
    }

    @VisibleForTesting
    synchronized ThumbnailEntry getMostRecentEntry() {
        if (getCacheCount() <= 0) return null;

        ArrayList<Map.Entry<String, ThumbnailEntry>> entryList =
                new ArrayList<Map.Entry<String, ThumbnailEntry>>(sDiskLruCache.entrySet());
        return entryList.get(entryList.size() - 1).getValue();
    }

    /**
     * The number of entries in the disk cache.
     */
    @VisibleForTesting
    synchronized int getCacheCount() {
        return sDiskLruCache.size();
    }
}