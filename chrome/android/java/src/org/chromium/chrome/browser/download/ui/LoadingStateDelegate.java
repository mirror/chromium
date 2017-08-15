// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

/**
 * Determines when the data from all of the backends has been loaded.
 * <p>
 * TODO(ianwen): add a timeout mechanism to either the DownloadLoadingDelegate or to the
 * backend so that if it takes forever to load one of the backend, users are still able to see
 * the other two.
 */
public class LoadingStateDelegate {
    private boolean mLoadingState;
    private int mPendingFilter = DownloadFilter.FILTER_ALL;

    /**
     * Tells this delegate one of the three backends has been loaded.
     * @return Whether or not the backends are all loaded.
     */
    public boolean updateLoadingState(boolean loaded) {
        mLoadingState = loaded;
        return isLoaded();
    }

    /** @return Whether all backends are loaded. */
    public boolean isLoaded() {
        return mLoadingState;
    }

    /** Caches a filter for when the backends have loaded. */
    public void setPendingFilter(int filter) {
        mPendingFilter = filter;
    }

    /** @return The cached filter, or {@link DownloadFilter#FILTER_ALL} if none was set. */
    public int getPendingFilter() {
        return mPendingFilter;
    }
}
