// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnerbookmarks;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import org.chromium.base.VisibleForTesting;

import java.util.HashMap;
import java.util.Map;

/**
 * A cache for storing failed favicon loads along with a timestamp for the last failed retrieval,
 * so we can throttle the number of requests made for previously failed favicon fetch attempts.
 */
public class PartnerBookmarksCache {
    private static final String PREFERENCES_NAME = "partner_bookmarks_favicon_cache";

    private static final String KEY_FAVICON_COUNT = "favicon_count";
    private static final String KEY_FAVICON_URL_PREFIX = "favicon_url";
    private static final String KEY_FAVICON_TIMESTAMP_PREFIX = "favicon_timestamp";

    private final SharedPreferences mSharedPreferences;

    public PartnerBookmarksCache(Context context) {
        this(context, PREFERENCES_NAME);
    }

    public PartnerBookmarksCache(Context context, String cacheName) {
        mSharedPreferences = context.getSharedPreferences(cacheName, 0);
    }

    /**
     * Reads the favicon retrieval timestamp information from a cache-specific
     * {@link SharedPreferences}.
     *
     * @return A map with the favicon URLs and their last retrieved timestamps.
     */
    public Map<String, Long> read() {
        Map<String, Long> outMap = new HashMap<String, Long>();

        int count = mSharedPreferences.getInt(KEY_FAVICON_COUNT, 0);
        for (int i = 0; i < count; i++) {
            String url = mSharedPreferences.getString(KEY_FAVICON_URL_PREFIX + i, null);
            long timestamp = mSharedPreferences.getLong(KEY_FAVICON_TIMESTAMP_PREFIX + i, 0L);
            if (url != null) {
                outMap.put(url, timestamp);
            }
        }

        return outMap;
    }

    /**
     * Writes the provided map into the cache's {@link SharedPreferences}.
     * This overwrites what was previously in the cache.
     *
     * @param inMap The favicon/timestamps to write to cache.
     */
    public void write(Map<String, Long> inMap) {
        if (inMap == null)
            throw new IllegalArgumentException("PartnerBookmarksCache: write() "
                    + "input cannot be null.");

        int numEntries = inMap.keySet().size();
        Editor editor = mSharedPreferences.edit();
        editor.clear();
        editor.putInt(KEY_FAVICON_COUNT, numEntries);

        int i = 0;
        for (Map.Entry<String, Long> entry : inMap.entrySet()) {
            editor.putString(KEY_FAVICON_URL_PREFIX + i, entry.getKey());
            editor.putLong(KEY_FAVICON_TIMESTAMP_PREFIX + i, entry.getValue());
            i++;
        }
        editor.apply();
    }

    /**
     * Called after tests so we don't leave behind test {@link SharedPreferences}, and have data
     * from one test run into another.
     */
    @VisibleForTesting
    public void clearCache() {
        mSharedPreferences.edit().clear().apply();
    }
}