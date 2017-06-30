// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import android.view.ViewGroup;

import org.junit.Before;
import org.junit.Rule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.base.DiscardableReferencePool;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.ntp.cards.NodeParent;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for {@link TileGroup}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@Features(@Features.Register(ChromeFeatureList.NTP_OFFLINE_PAGES_FEATURE_NAME))
public class TileGridTest {
    @Rule
    public final SuggestionsDependenciesRule dependenciesRule = new SuggestionsDependenciesRule();

    @Mock
    private TileGroup.Observer mTileGroupObserver;

    private FakeTileGroupDelegate mTileGroupDelegate;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mTileGroupDelegate = spy(new FakeTileGroupDelegate());
    }

    // @Test // TODO(dgn) implement
    public void testLoadTileCompletion() {
        NodeParent parent = mock(NodeParent.class);
        SuggestionsUiDelegate uiDelegate = createUiDelegate();

        // create view holder as A) use dummy viewholder/layout or B) real view holder
        TileGrid.ViewHolder vh = new TileGrid.ViewHolder(mock(ViewGroup.class));

        // Start observing -> mTileGroupObserver set
        TileGrid tileGrid = new TileGrid(uiDelegate, /*ctxMenuManager*/ null, mTileGroupDelegate,
                /*OfflinePageBridge*/ null);
        tileGrid.setParent(parent);

        // Send initial tile data -> expect loadCompleted not called
        // expect parent notified with tile load callback

        // Call the load callback with VH
        // check if we have a load pending (?)
        // complete the image callbacks
        // verify it is completed.

        // simulate close/open bottom sheet.
        // verify we get a completed event
    }

    /**
     * Notifies the tile group of new tiles created from the provided URLs. Requires
     * {@link TileGroup#startObserving(int)} to have been called on the tile group under test.
     * @see TileGroup#onMostVisitedURLsAvailable(String[], String[], String[], int[])
     */
    private void notifyTileUrlsAvailable(String... urls) {
        assertNotNull("MostVisitedObserver has not been registered.", mTileGroupDelegate.mObserver);

        String[] titles = urls.clone();
        String[] whitelistIconPaths = new String[urls.length];
        int[] sources = new int[urls.length];
        for (int i = 0; i < urls.length; ++i) whitelistIconPaths[i] = "";
        mTileGroupDelegate.mObserver.onMostVisitedURLsAvailable(
                titles, urls, whitelistIconPaths, sources);
    }

    private SuggestionsUiDelegate createUiDelegate() {
        Profile profile = mock(Profile.class);
        return new SuggestionsUiDelegateImpl(new FakeSuggestionsSource(),
                mock(SuggestionsEventReporter.class), mock(SuggestionsNavigationDelegate.class),
                profile, mock(NativePageHost.class), new DiscardableReferencePool());
    }

    private class FakeTileGroupDelegate implements TileGroup.Delegate {
        public MostVisitedSites.Observer mObserver;

        @Override
        public void removeMostVisitedItem(Tile tile, Callback<String> removalUndoneCallback) {}

        @Override
        public void openMostVisitedItem(int windowDisposition, Tile tile) {}

        @Override
        public void setMostVisitedSitesObserver(
                MostVisitedSites.Observer observer, int maxResults) {
            mObserver = observer;
        }

        @Override
        public void onLoadingComplete(Tile[] tiles) {}

        @Override
        public void destroy() {}
    }
}
