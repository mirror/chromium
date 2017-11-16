// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnerbookmarks;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * A cache for storing failed favicon loads along with a timestamp determining what point we can
 * attempt retrieval again, as a way of throttling the number of requests made for previously failed
 * favicon fetch attempts.
 */
public class PartnerBookmarksFaviconThrottle {
    private static final String PREFERENCES_NAME = "partner_bookmarks_favicon_cache";
    private static final long FAVICON_RETRIEVAL_TIMEOUT_MS = TimeUnit.DAYS.toMillis(30);

    private final SharedPreferences mSharedPreferences;

    // A map storing the current values in the cache at the time of initialization.
    private Map<String, Long> mCurrentCache;

    // Stores the values that need to be persisted in the cache at the end of this session.
    private Map<String, Long> mNewCache;

    public PartnerBookmarksFaviconThrottle(Context context) {
        this(context, PREFERENCES_NAME);
    }

    @VisibleForTesting
    PartnerBookmarksFaviconThrottle(Context context, String cacheName) {
        mSharedPreferences = context.getSharedPreferences(cacheName, 0);
        init();
    }

    /**
     * Reads the favicon retrieval timestamp information from a cache-specific
     * {@link SharedPreferences}. Must be called before we start retrieving favicons, or
     * {@link #getExpiryOf} will do nothing.
     *
     * Suppressing "unchecked" because we're 100% sure we're storing only <String, Long> pairs in
     * our cache.
     */
    @SuppressWarnings("unchecked")
    @VisibleForTesting
    void init() {
        mCurrentCache = (Map<String, Long>) mSharedPreferences.getAll();
        RecordHistogram.recordCount100Histogram(
                "PartnerBookmarksCache.CacheSize", mCurrentCache.size());
        mNewCache = new HashMap<String, Long>();
    }

    /**
     * Writes the new map that was built as a result of the calls to {@link #onFaviconFetch} to disk
     * in our {@link SharedPreferences}. This overwrites what was previously in the cache.
     */
    public void commit() {
        assert mNewCache != null;

        // Save ourselves a write to disk if the two caches are identical.
        if (mCurrentCache.equals(mNewCache)) {
            return;
        }

        Editor editor = mSharedPreferences.edit();
        editor.clear();
        for (Map.Entry<String, Long> entry : mNewCache.entrySet()) {
            editor.putLong(entry.getKey(), entry.getValue());
        }
        editor.apply();
    }

    /**
     * Calling this with each favicon fetch URL and result builds the new output cache to be
     * written to disk when {@link #commit} is called.
     *
     * @param url The page URL we attempted to fetch a favicon for.
     * @param result The {@link FaviconFetchResult} response we got for this URL.
     */
    public void onFaviconFetched(String url, @FaviconFetchResult int result) {
        assert mCurrentCache != null;
        assert mNewCache != null;

        if (result == FaviconFetchResult.FAILURE_SERVER_ERROR) {
            mNewCache.put(url, System.currentTimeMillis() + FAVICON_RETRIEVAL_TIMEOUT_MS);
        } else if (result != FaviconFetchResult.SUCCESS && mCurrentCache.containsKey(url)
                && (System.currentTimeMillis() < mCurrentCache.get(url))) {
            // Keep a URL in the cache if it hasn't yet expired and we get didn't just get a
            // success response.
            mNewCache.put(url, getExpiryOf(url));
        }
    }

    /**
     * Determines, based on the contents of our cache, whether or not we should even attempt to
     * reach out to a server to retrieve a favicon that we don't currently have cached.
     *
     * @param url The page URL we need a favicon for.
     * @return Whether or not we should fetch the favicon from server if necessary.
     */
    public boolean shouldFetchFromServerIfNecessary(String url) {
        Long expiryTimeMs = getExpiryOf(url);
        return expiryTimeMs == null || System.currentTimeMillis() >= expiryTimeMs;
    }

    /**
     * Gets the expiry time in ms of a particular URL for which we're fetching a favicon. Cached
     * URLs that have previously failed to retrieve a favicon from a server will have a value at
     * which point we should attempt a retrieval again, otherwise we return null for entries
     * not in the cahce.
     *
     * @param url The page URL we're trying to fetch a favicon for.
     * @return The expiry time of the favicon fetching restriction in ms if the entry exists in the
     *         cache, otherwise null.
     */
    private Long getExpiryOf(String url) {
        assert mCurrentCache != null;

        if (mCurrentCache.containsKey(url)) {
            return mCurrentCache.get(url);
        }
        return null;
    }

    /**
     * Called after tests so we don't leave behind test {@link SharedPreferences}, and have data
     * from one test run into another.
     */
    @VisibleForTesting
    void clearCache() {
        mSharedPreferences.edit().clear().apply();
    }

    /**
     * @return Number of entries in the cache.
     */
    @VisibleForTesting
    int numEntries() {
        assert mCurrentCache != null;
        return mCurrentCache.size();
    }
}