// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnerbookmarks;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.support.annotation.VisibleForTesting;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A cache for partner bookmarks and associated information, backed by an SQLite database.
 *
 * The information associated with the bookamrks includes:
 *      * Page URL.
 *      * The last time a favicon retrieval from a Google server was attempted.
 *      * How many times we failed to retrieve that favicon.
 */
public class PartnerBookmarksCache extends SQLiteOpenHelper {
    public static final int DATABASE_VERSION = 1;

    @VisibleForTesting
    public static final String DATABASE_NAME = "partnerbookmarks.db";

    private static final String PARTNER_BOOKMARKS_TABLE_NAME = "partnerbookmarks";
    private static final String URL_COLUMN = "url";
    private static final String LAST_FAVICON_RETRIEVAL_TIME_MS_COLUMN =
            "last_favicon_retrieval_time_ms";
    private static final String FAVICON_FAILED_ATTEMPTS_COLUMN = "favicon_failed_attempts";

    private static PartnerBookmarksCache sInstance;

    private final Map<String, CachedBookmark> mBookmarks = new HashMap<String, CachedBookmark>();

    private SQLiteDatabase mDatabase;

    /**
     * A structure that represents the items in the cache.
     */
    public static class CachedBookmark {
        private final String mUrl;
        private final long mLastRetrievalTimeMs;
        private final int mNumFailedAttempts;

        public CachedBookmark(String url, long lastRetrievalTimeMs, int numFailedAttempts) {
            mUrl = url;
            mLastRetrievalTimeMs = lastRetrievalTimeMs;
            mNumFailedAttempts = numFailedAttempts;
        }

        public String url() {
            return mUrl;
        }
        public long lastRetrievalTimeMs() {
            return mLastRetrievalTimeMs;
        }
        public int numFailedAttempts() {
            return mNumFailedAttempts;
        }
    }

    public PartnerBookmarksCache(Context context) {
        this(context, DATABASE_NAME);
    }

    public PartnerBookmarksCache(Context context, String databaseName) {
        super(context, databaseName, null, DATABASE_VERSION);
        mDatabase = getWritableDatabase();
    }

    @Override
    public synchronized void close() {
        super.close();
        mDatabase = null;
    }

    /**
     * Gets a singleton instance of {@link PartnerBookmarksCache}.
     *
     * @param context An Android application context.
     * @return A singleton instance of the {@link PartnerBookmarksCache}.
     */
    public static PartnerBookmarksCache getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new PartnerBookmarksCache(context, DATABASE_NAME);
        }
        return sInstance;
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        createTable(db);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        clearCache(db);
    }

    @Override
    public void onOpen(SQLiteDatabase db) {
        String[] projection = {
                URL_COLUMN, LAST_FAVICON_RETRIEVAL_TIME_MS_COLUMN, FAVICON_FAILED_ATTEMPTS_COLUMN};

        // Grab everything from the database and load it into our hash map so we don't have to go to
        // disk to check every bookmark.
        Cursor cursor =
                db.query(PARTNER_BOOKMARKS_TABLE_NAME, projection, null, null, null, null, null);
        while (cursor.moveToNext()) {
            String url = cursor.getString(cursor.getColumnIndexOrThrow(URL_COLUMN));
            long lastRetrievalTimeMs = cursor.getLong(
                    cursor.getColumnIndexOrThrow(LAST_FAVICON_RETRIEVAL_TIME_MS_COLUMN));
            int numFailedAttempts =
                    cursor.getInt(cursor.getColumnIndexOrThrow(FAVICON_FAILED_ATTEMPTS_COLUMN));

            CachedBookmark bookmark =
                    new CachedBookmark(url, lastRetrievalTimeMs, numFailedAttempts);
            mBookmarks.put(url, bookmark);
        }
    }

    /**
     * Helper function that creates {@link ContentValues} for the given {@link CachedBookmark} to be
     * used in database queries.
     *
     * @param bookmark The {@link CachedBookmark} to get {@link ContentValues} for.
     * @return The resulting {@link ContentValues}.
     */
    private ContentValues contentValuesFromBookmark(CachedBookmark bookmark) {
        ContentValues values = new ContentValues();
        values.put(URL_COLUMN, bookmark.url());
        values.put(LAST_FAVICON_RETRIEVAL_TIME_MS_COLUMN, bookmark.lastRetrievalTimeMs());
        values.put(FAVICON_FAILED_ATTEMPTS_COLUMN, bookmark.numFailedAttempts());
        return values;
    }

    /**
     * Adds a {@link CachedBookmark} to the database and in-memory hash map.
     *
     * @param bookmark The {@link CachedBookmark} to store.
     * @return Whether or not the bookmark was successfully added to the database.
     */
    public boolean addBookmark(CachedBookmark bookmark) {
        if (mBookmarks.containsKey(bookmark.url())) {
            return false;
        }

        ContentValues values = contentValuesFromBookmark(bookmark);
        if (mDatabase.insert(PARTNER_BOOKMARKS_TABLE_NAME, null, values) >= 0) {
            mBookmarks.put(bookmark.url(), bookmark);
            return true;
        }
        return false;
    }

    /**
     * Updates a {@link CachedBookmark} in the database if the URL of the bookmark already exists.
     *
     * @param bookmark The {@link CachedBookmark} information we want updated in the database.
     * @return Whether or not the bookmark was successfully updated.
     */
    public boolean updateBookmark(CachedBookmark bookmark) {
        if (!mBookmarks.containsKey(bookmark.url())) {
            return false;
        }

        ContentValues values = contentValuesFromBookmark(bookmark);
        String where = URL_COLUMN + " = ?";
        String[] whereArgs = {bookmark.url()};
        if (mDatabase.update(PARTNER_BOOKMARKS_TABLE_NAME, values, where, whereArgs) == 0) {
            // TODO(thildebr): Throw an exception? Our map said we had this URL, but we failed to
            // update it.
            return false;
        }

        mBookmarks.put(bookmark.url(), bookmark);
        return true;
    }

    /** Retrieves the {@link CachedBookmark} from our cache for the provided URL, if it exists.
     *
     * @param url The bookmark URL.
     * @return A {@link CachedBookmark} for the provided URL, or null if it doesn't exist.
     */
    public CachedBookmark getBookmarkForUrl(String url) {
        if (mBookmarks.containsKey(url)) {
            return mBookmarks.get(url);
        }
        return null;
    }

    /**
     * Initializes our partner bookmarks SQLite table.
     *
     * @param db The SQLite database.
     */
    private void createTable(SQLiteDatabase db) {
        String createTableQuery = "CREATE TABLE " + PARTNER_BOOKMARKS_TABLE_NAME + " ("
                + "id INTEGER PRIMARY KEY"
                + "," + URL_COLUMN + " TEXT UNIQUE ON CONFLICT REPLACE"
                + "," + LAST_FAVICON_RETRIEVAL_TIME_MS_COLUMN + " INTEGER"
                + "," + FAVICON_FAILED_ATTEMPTS_COLUMN + " INTEGER)";
        db.execSQL(createTableQuery);
    }

    /**
     * Clears the cache by simply deleting the table from the database and re-creating it.
     *
     * @param db The SQLite database.
     */
    public void clearCache(SQLiteDatabase db) {
        db.execSQL("DROP TABLE IF EXISTS partnerbookmarks");
        createTable(db);
        mBookmarks.clear();
    }

    @VisibleForTesting
    public List<CachedBookmark> getBookmarksList() {
        List<CachedBookmark> outList = new ArrayList<CachedBookmark>();
        for (CachedBookmark bookmark : mBookmarks.values()) {
            outList.add(bookmark);
        }
        return outList;
    }
}