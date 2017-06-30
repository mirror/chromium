// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import static org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils.createDummySuggestion;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.DisableHistogramsRule;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider;
import org.chromium.chrome.browser.favicon.FaviconHelper;
import org.chromium.chrome.browser.favicon.FaviconHelper.FaviconImageCallback;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.chrome.browser.ntp.cards.CardsVariationParameters;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.ImageFetcher.DownloadThumbnailRequest;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.net.URI;
import java.util.HashMap;
/**
 * Unit tests for {@link ImageFetcher}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ImageFetcherTest {
    public static final int IMAGE_SIZE_PX = 100;
    public static final String URL_STRING = "http://www.test.com";
    @Rule
    public DisableHistogramsRule disableHistogramsRule = new DisableHistogramsRule();

    @Rule
    public SuggestionsDependenciesRule mSuggestionsDeps = new SuggestionsDependenciesRule();

    @Mock
    FaviconHelper mFaviconHelper;
    @Mock
    private SuggestionsUiDelegate mUiDelegate;
    @Mock
    private ThumbnailProvider mThumbnailProvider;
    @Mock
    private SuggestionsSource mSuggestionsSource;
    @Mock
    private LargeIconBridge mLargeIconBridge;

    private ImageFetcher mImageFetcher;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        CardsVariationParameters.setTestVariationParams(new HashMap<String, String>());
        mImageFetcher = new ImageFetcher(
                mSuggestionsSource, mock(Profile.class), mock(NativePageHost.class));

        mSuggestionsDeps.getFactory().largeIconBridge = mLargeIconBridge;
        mSuggestionsDeps.getFactory().thumbnailProvider = mThumbnailProvider;
        mSuggestionsDeps.getFactory().faviconHelper = mFaviconHelper;

        when(mUiDelegate.getSuggestionsSource()).thenReturn(mSuggestionsSource);
    }

    @Test
    public void testFaviconFetch() {
        SnippetArticle suggestion = createDummySuggestion(KnownCategories.BOOKMARKS);
        mImageFetcher.makeFaviconRequest(suggestion, IMAGE_SIZE_PX, mock(Callback.class));

        String expectedFaviconDomain =
                ImageFetcher.getSnippetDomain(URI.create(suggestion.getUrl()));
        verify(mFaviconHelper)
                .getLocalFaviconImageForURL(any(Profile.class), eq(expectedFaviconDomain), anyInt(),
                        any(FaviconImageCallback.class));
    }

    @Test
    public void testDownloadThumbnailFetch() {
        SnippetArticle suggestion = createDummySuggestion(KnownCategories.DOWNLOADS);

        DownloadThumbnailRequest request = mImageFetcher.makeDownloadThumbnailRequest(
                suggestion, IMAGE_SIZE_PX, mock(Callback.class));
        verify(mThumbnailProvider).getThumbnail(eq(request));

        request.cancel();
        verify(mThumbnailProvider).cancelRetrieval(eq(request));
    }

    @Test
    public void testArticleThumbnailFetch() {
        SnippetArticle suggestion = createDummySuggestion(KnownCategories.ARTICLES);
        mImageFetcher.makeArticleThumbnailRequest(suggestion, mock(Callback.class));

        verify(mUiDelegate.getSuggestionsSource())
                .fetchSuggestionImage(eq(suggestion), any(Callback.class));
    }

    @Test
    public void testLargeIconFetch() {
        mImageFetcher.makeLargeIconRequest(
                URL_STRING, IMAGE_SIZE_PX, mock(LargeIconCallback.class));

        String expectedIconDomain = ImageFetcher.getSnippetDomain(URI.create(URL_STRING));
        verify(mLargeIconBridge)
                .getLargeIconForUrl(eq(expectedIconDomain), anyInt(), any(LargeIconCallback.class));
    }

    @Test
    public void testSnippetDomainExtraction() {
        URI uri = URI.create(URL_STRING + "/test");

        String expected = URL_STRING;
        String actual = ImageFetcher.getSnippetDomain(uri);

        assertEquals(expected, actual);
    }
}
