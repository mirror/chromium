// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import org.chromium.base.ThreadUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A fake implementation of MostVisitedSites that returns a fixed list of most visited sites.
 */
public class FakeMostVisitedSites implements MostVisitedSites {
    private final List<String> mBlacklistedUrls = new ArrayList<>();

    private List<SiteSuggestion> mSites = new ArrayList<>();
    private Observer mObserver;

    @Override
    public void destroy() {}

    @Override
    public void setObserver(Observer observer, int numResults) {
        mObserver = observer;
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

    /**
     * @return Whether blacklistUrl() has been called on the given url.
     */
    public boolean isUrlBlacklisted(String url) {
        return mBlacklistedUrls.contains(url);
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
        SiteSuggestion[] suggestions = new SiteSuggestion[urls.length];
        for (int i = 0; i < urls.length; ++i) suggestions[i] = createSiteSuggestion(urls[i]);

        setTileSuggestions(suggestions);
    }

    public static SiteSuggestion createSiteSuggestion(String url) {
        return createSiteSuggestion(url, url);
    }

    public static SiteSuggestion createSiteSuggestion(String title, String url) {
        return new SiteSuggestion(title, url, "", TileSource.TOP_SITES);
    }

    private void notifyTileSuggestionsAvailable() {
        if (mObserver == null) return;
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mObserver.onSiteSuggestionsAvailable(mSites);
            }
        });
    }
}
