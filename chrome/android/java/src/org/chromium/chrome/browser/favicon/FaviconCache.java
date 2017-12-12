// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.favicon;

import android.app.ActivityManager;
import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.HashSet;
import java.util.Set;

/**
 * A class to cache favicons, backed by a {@link LargeIconBridge}.
 *
 * Using {@link FaviconCache.Manager} to create a {@link FaviconCache} ties it to the
 * {@link Manager} so we can notify all of the caches of favicon updates, which will then in turn
 * notify their observers. This is useful for things like refreshing a view once favicons have
 * finished asynchronously loading in the background.
 */
public class FaviconCache {
    private LargeIconBridge mLargeIconBridge;
    private Set<Observer> mObservers = new HashSet<>();

    /**
     * A static manager class for creating and notifying a global list of {@link FaviconCache}s of
     * changes to favicons.
     */
    public static class Manager {
        private static Set<FaviconCache> sCacheSet = new HashSet<>();

        /**
         * Creates a {@link FaviconCache} and ties it to this {@link Manager}
         *
         * @param size Maximum size of the cache in bytes.
         * @return A new instance of {@link FaviconCache}.
         */
        public static FaviconCache create(int size) {
            FaviconCache cache = new FaviconCache(size);
            sCacheSet.add(cache);
            return cache;
        }

        /**
         * Removes a {@link FaviconCache} from the set that receives updates from the
         * {@link Manager}.
         *
         * @param cache The {@link FaviconCache} to remove from the {@link Manager}. This means
         *        the cache will no longer be notified of changes signaled through the
         *        {@link Manager}.
         */
        public static void remove(FaviconCache cache) {
            sCacheSet.remove(cache);
        }

        /**
         * Called when a favicon has been updated elsewhere, so the local cache needs to refresh
         * itself as well.
         *
         * @param url The URL of the page for the favicon being updated.
         */
        public static void updateFavicon(String url) {
            for (FaviconCache cache : sCacheSet) {
                cache.onUpdateFavicon(url);
            }
        }
    }

    /**
     * Observer for listeners to receive updates when changes are made to the favicon cache.
     */
    public interface Observer {
        /**
         * Called when a favicon has been updated, so observers can update their view if necessary.
         *
         * @param url The URL of the page for the favicon being updated.
         */
        void onUpdateFavicon(String url);
    }

    private FaviconCache(int size) {
        mLargeIconBridge = new LargeIconBridge(Profile.getLastUsedProfile().getOriginalProfile());
        ActivityManager activityManager =
                ((ActivityManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.ACTIVITY_SERVICE));
        int maxSize = Math.min(activityManager.getMemoryClass() / 4 * 1024 * 1024, size);
        mLargeIconBridge.createCache(maxSize);
    }

    /**
     * Called when the {@link Manager} has been told of a favicon update.
     *
     * @param url The URL of the page for the favicon being updated.
     */
    private void onUpdateFavicon(String url) {
        // TODO(thildebr): Figure out why remove() in the LruCache in LargeIconBridge doesn't
        // permanently remove items, and remove only those here. That way we can avoid clearing the
        // entire cache when refreshing bookmarks.
        for (Observer observer : mObservers) {
            observer.onUpdateFavicon(url);
        }
    }

    public void addObserver(Observer observer) {
        mObservers.add(observer);
    }

    public void removeObserver(Observer observer) {
        mObservers.remove(observer);
    }

    public void destroy() {
        mLargeIconBridge.destroy();
        Manager.remove(this);
    }

    public void clear() {
        mLargeIconBridge.clearCache();
    }

    public LargeIconBridge getLargeIconBridge() {
        return mLargeIconBridge;
    }
}