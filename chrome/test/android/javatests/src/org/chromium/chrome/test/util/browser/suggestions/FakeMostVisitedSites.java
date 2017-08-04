// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.suggestions;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.suggestions.MostVisitedSites;
import org.chromium.chrome.browser.suggestions.SiteSuggestion;
import org.chromium.chrome.browser.suggestions.TileSource;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A fake implementation of MostVisitedSites that returns a fixed list of most visited sites.
 */
public class FakeMostVisitedSites implements MostVisitedSites {
    private final List<String> mBlacklistedUrls = new ArrayList<>();
    private final boolean mObserveOnUiThread;

    private List<SiteSuggestion> mSites = new ArrayList<>();
    private Observer mObserver;

    /**
     * Creates a FakeMostVisitedSites that does not force its observer to return on the UI thread.
     * @see #FakeMostVisitedSites(boolean)
     */
    public FakeMostVisitedSites() {
        this(false);
    }

    /**
     * @param observeOnUiThread whether registered observers should be forced to return on the
     *      UI thread. Useful for instrumentation tests that run by default on a separate thread but
     *      where observers do view manipulations and thus should run on the UI thread.
     * @see #setObserver(Observer, int)
     * @see MostVisitedSites.Observer
     */
    public FakeMostVisitedSites(boolean observeOnUiThread) {
        mObserveOnUiThread = observeOnUiThread;
    }

    @Override
    public void destroy() {}

    @Override
    public void setObserver(Observer observer, int numResults) {
        mObserver = mObserveOnUiThread ? new UiThreadObserver(observer) : observer;
        notifyTileSuggestionsAvailable();
    }

    @Override
    public void addBlacklistedUrl(String url) {
        mBlacklistedUrls.add(url);
    }

    @Override
    public void removeBlacklistedUrl(String url) {
        mBlacklistedUrls.remove(url);
    }

    @Override
    public void recordPageImpression(int tilesCount) {
        // Metrics are stubbed out.
    }

    @Override
    public void recordTileImpression(int index, int type, int source, String url) {
        // Metrics are stubbed out.
    }

    @Override
    public void recordOpenedMostVisitedItem(int index, int tileType, int source) {
        //  Metrics are stubbed out.
    }

    /** @return Whether {@link #addBlacklistedUrl} has been called on the given URL. */
    public boolean isUrlBlacklisted(String url) {
        return mBlacklistedUrls.contains(url);
    }

    /** Sets new tile suggestion data. If there is an observer it is notified. */
    public void setTileSuggestions(List<SiteSuggestion> suggestions) {
        mSites = new ArrayList<>(suggestions);
        notifyTileSuggestionsAvailable();
    }

    /** Sets new tile suggestion data. If there is an observer it is notified. */
    public void setTileSuggestions(SiteSuggestion... suggestions) {
        setTileSuggestions(Arrays.asList(suggestions));
    }

    /**
     * Sets new tile suggestion data, generating dummy data for the missing properties.
     * If there is an observer it is notified.
     *
     * @param urls The URLs of the site suggestions.
     * @see #setTileSuggestions(SiteSuggestion[])
     */
    public void setTileSuggestions(String... urls) {
        setTileSuggestions(createSiteSuggestions(urls));
    }

    /** @return An unmodifiable view of the current list of sites. */
    public List<SiteSuggestion> getCurrentSites() {
        return Collections.unmodifiableList(mSites);
    }

    public static List<SiteSuggestion> createSiteSuggestions(String... urls) {
        List<SiteSuggestion> suggestions = new ArrayList<>(urls.length);
        for (String url : urls) suggestions.add(createSiteSuggestion(url));
        return suggestions;
    }

    public static SiteSuggestion createSiteSuggestion(String url) {
        return createSiteSuggestion(url, url);
    }

    public static SiteSuggestion createSiteSuggestion(String title, String url) {
        return new SiteSuggestion(title, url, "", TileSource.TOP_SITES);
    }

    private void notifyTileSuggestionsAvailable() {
        if (mObserver == null) return;
        mObserver.onSiteSuggestionsAvailable(mSites);
    }

    /**
     * Implementation of {@link MostVisitedSites.Observer} that wraps another one to ensure its
     * methods run on the UI thread.
     */
    private static class UiThreadObserver implements Observer {
        private final Observer mWrappedObserver;

        private UiThreadObserver(Observer wrappedObserver) {
            mWrappedObserver = wrappedObserver;
        }

        @Override
        public void onSiteSuggestionsAvailable(final List<SiteSuggestion> siteSuggestions) {
            ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    mWrappedObserver.onSiteSuggestionsAvailable(siteSuggestions);
                }
            });
        }

        @Override
        public void onIconMadeAvailable(final String siteUrl) {
            ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    mWrappedObserver.onIconMadeAvailable(siteUrl);
                }
            });
        }
    }
}
